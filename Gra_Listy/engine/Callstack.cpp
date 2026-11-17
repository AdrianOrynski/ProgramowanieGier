#include "engine/Callstack.hpp"

#include <spdlog/fmt/fmt.h>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <DbgHelp.h>    // SymInitialize, SymFromAddr, SymGetLineFromAddr64 (po Windows.h)

    #pragma comment(lib, "dbghelp.lib")
#endif

namespace Engine
{
#if defined(_WIN32)
    namespace
    {
        // Inicjalizacja DbgHelp raz na proces. fInvadeProcess = TRUE ładuje symbole
        // wszystkich załadowanych modułów (z SYMOPT_DEFERRED_LOADS dopiero przy pierwszym użyciu).
        bool EnsureSymbolsInitialized(HANDLE process)
        {
            static const bool initialized = [process]
            {
                SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
                return SymInitialize(process, nullptr, TRUE) == TRUE;
            }();
            return initialized;
        }
    }

    std::vector<SStackFrame> CaptureCallstack(unsigned int skipFrames, unsigned int maxFrames)
    {
        // CaptureStackBackTrace przyjmuje maksymalnie 62 ramki (suma skip + count < 63 na starszych systemach)
        void* addresses[62];
        if (maxFrames > 62)
            maxFrames = 62;

        // +1 pomija ramkę samej CaptureCallstack
        const USHORT count = CaptureStackBackTrace(skipFrames + 1, maxFrames, addresses, nullptr);

        std::vector<SStackFrame> frames;
        frames.reserve(count);

        const HANDLE process = GetCurrentProcess();
        const bool hasSymbols = EnsureSymbolsInitialized(process);

        for (USHORT i = 0; i < count; ++i)
        {
            SStackFrame frame;
            frame.address = reinterpret_cast<std::uintptr_t>(addresses[i]);

            if (hasSymbols)
            {
                const DWORD64 address = static_cast<DWORD64>(frame.address);

                // SYMBOL_INFO ma na końcu tablicę Name[1] - bufor musi zmieścić całą nazwę
                alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
                auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen = MAX_SYM_NAME;

                DWORD64 symbolDisplacement = 0;
                if (SymFromAddr(process, address, &symbolDisplacement, symbol))
                    frame.function = symbol->Name;

                IMAGEHLP_LINE64 lineInfo{};
                lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD lineDisplacement = 0;
                if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo))
                {
                    frame.file = lineInfo.FileName;
                    frame.line = lineInfo.LineNumber;
                }
            }

            frames.push_back(std::move(frame));
        }

        return frames;
    }
#else
    std::vector<SStackFrame> CaptureCallstack(unsigned int, unsigned int)
    {
        // Brak implementacji poza Windows (można dodać backtrace() z <execinfo.h>)
        return {};
    }
#endif

    std::string FormatStackFrame(const SStackFrame& frame)
    {
        const std::string function = frame.function.empty() ? "<nieznana funkcja>" : frame.function;

        if (frame.file.empty())
            return fmt::format("{} [0x{:X}]", function, frame.address);

        return fmt::format("{} ({}:{}) [0x{:X}]", function, frame.file, frame.line, frame.address);
    }
}
