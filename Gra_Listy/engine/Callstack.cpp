#include "engine/Callstack.hpp"

#include <utility>

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

        // Adres -> nazwa funkcji, plik i linia (o ile są symbole debugowe)
        SStackFrame SymbolizeAddress(HANDLE process, bool hasSymbols, std::uintptr_t address)
        {
            SStackFrame frame;
            frame.address = address;

            if (!hasSymbols)
                return frame;

            const DWORD64 address64 = static_cast<DWORD64>(address);

            // SYMBOL_INFO ma na końcu tablicę Name[1] - bufor musi zmieścić całą nazwę
            alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(char)];
            auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 symbolDisplacement = 0;
            if (SymFromAddr(process, address64, &symbolDisplacement, symbol))
                frame.function = symbol->Name;

            IMAGEHLP_LINE64 lineInfo{};
            lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD lineDisplacement = 0;
            if (SymGetLineFromAddr64(process, address64, &lineDisplacement, &lineInfo))
            {
                frame.file = lineInfo.FileName;
                frame.line = lineInfo.LineNumber;
            }

            return frame;
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
            frames.push_back(SymbolizeAddress(process, hasSymbols, reinterpret_cast<std::uintptr_t>(addresses[i])));

        return frames;
    }

    std::vector<SStackFrame> CaptureCallstackFromContext(const void* platformContext, unsigned int maxFrames)
    {
        std::vector<SStackFrame> frames;
        if (platformContext == nullptr)
            return frames;

        // StackWalk64 modyfikuje kontekst w trakcie rozwijania stosu - pracujemy na kopii
        CONTEXT context = *static_cast<const CONTEXT*>(platformContext);

        // Punkt startowy: licznik instrukcji, wskaźnik ramki i wskaźnik stosu z chwili wyjątku
        STACKFRAME64 stackFrame{};
        DWORD machineType = 0;
#if defined(_M_X64)
        machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrFrame.Offset = context.Rbp;
        stackFrame.AddrStack.Offset = context.Rsp;
#elif defined(_M_IX86)
        machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrFrame.Offset = context.Ebp;
        stackFrame.AddrStack.Offset = context.Esp;
#elif defined(_M_ARM64)
        machineType = IMAGE_FILE_MACHINE_ARM64;
        stackFrame.AddrPC.Offset = context.Pc;
        stackFrame.AddrFrame.Offset = context.Fp;
        stackFrame.AddrStack.Offset = context.Sp;
#else
        return frames;
#endif
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        const HANDLE process = GetCurrentProcess();
        const HANDLE thread = GetCurrentThread();
        const bool hasSymbols = EnsureSymbolsInitialized(process);

        // SymFunctionTableAccess64 / SymGetModuleBase64 dają StackWalk64 dostęp do informacji
        // o rozwijaniu stosu (x64 nie używa łańcucha wskaźników ramek jak x86)
        while (frames.size() < maxFrames &&
               StackWalk64(machineType, process, thread, &stackFrame, &context,
                           nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
        {
            if (stackFrame.AddrPC.Offset == 0)
                break;

            frames.push_back(SymbolizeAddress(process, hasSymbols, static_cast<std::uintptr_t>(stackFrame.AddrPC.Offset)));
        }

        return frames;
    }
#else
    std::vector<SStackFrame> CaptureCallstack(unsigned int, unsigned int)
    {
        // Brak implementacji poza Windows (można dodać backtrace() z <execinfo.h>)
        return {};
    }

    std::vector<SStackFrame> CaptureCallstackFromContext(const void*, unsigned int)
    {
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
