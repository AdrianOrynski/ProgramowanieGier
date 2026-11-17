#include "engine/Assert.hpp"

#include <cstdlib>

#include "engine/Callstack.hpp"
#include "engine/Logger.hpp"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>    // IsDebuggerPresent
#else
    #include <csignal>
#endif

namespace Engine
{
    namespace
    {
        void TriggerBreakpoint()
        {
#if defined(_WIN32)
            // Bez debuggera __debugbreak() kończy się nieobsłużonym wyjątkiem -
            // wtedy wystarczy std::abort() poniżej
            if (IsDebuggerPresent())
                __debugbreak();
#else
            std::raise(SIGTRAP);
#endif
        }
    }

    void HandleAssertionFailure(std::string_view condition, std::string_view message,
                                const char* file, int line, const char* function)
    {
        spdlog::logger* logger = CLogger::Get();

        // source_loc miejsca asercji, a nie tej funkcji - [plik:linia] w patternie wskaże ASSERT
        const spdlog::source_loc location{ file, line, function };

        logger->log(location, spdlog::level::critical, "ASSERTION FAILED: {}", condition);
        logger->log(location, spdlog::level::critical, "  Wiadomosc: {}", message);
        logger->log(location, spdlog::level::critical, "  Plik:      {}", file);
        logger->log(location, spdlog::level::critical, "  Linia:     {}", line);
        logger->log(location, spdlog::level::critical, "  Funkcja:   {}", function);

        // (e) Backtrace - pomijamy ramkę HandleAssertionFailure, pierwsza będzie funkcja z ASSERT
        const std::vector<SStackFrame> callstack = CaptureCallstack(1);
        if (!callstack.empty())
        {
            logger->log(location, spdlog::level::critical, "  Stos wywolan:");
            for (std::size_t i = 0; i < callstack.size(); ++i)
                logger->log(location, spdlog::level::critical, "    #{:<2} {}", i, FormatStackFrame(callstack[i]));
        }

        // Program zaraz się zatrzyma - logi muszą trafić na dysk teraz
        logger->flush();

        TriggerBreakpoint();
        std::abort();
    }
}
