// Crash handler dla Windows: SetUnhandledExceptionFilter + MiniDumpWriteDump (DbgHelp)
#if defined(_WIN32)

#include "engine/CrashHandler.hpp"

#include <atomic>
#include <cwchar>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>    // MiniDumpWriteDump (po Windows.h)

#pragma comment(lib, "dbghelp.lib")

#include "engine/Callstack.hpp"
#include "engine/Logger.hpp"
#include "engine/Paths.hpp"

namespace Engine
{
    namespace
    {
        // Kod wyjątku C++ rzuconego przez MSVC (throw bez pasującego catch)
        constexpr DWORD CPP_EXCEPTION_CODE = 0xE06D7363;

        // Zapas stosu dla handlera - przy przepełnieniu stosu bez niego nie da się nic zalogować
        constexpr ULONG STACK_GUARANTEE_BYTES = 64 * 1024;

        LPTOP_LEVEL_EXCEPTION_FILTER s_PreviousFilter = nullptr;
        bool s_IsInstalled = false;

        // Katalog zrzutów przygotowany przy instalacji - w handlerze unikamy alokacji
        // (sterta może być uszkodzona) i operacji na std::filesystem
        wchar_t s_DumpDirectory[MAX_PATH] = {};

        // Chroni przed ponownym wejściem (np. awaria wewnątrz samego handlera lub w kilku wątkach naraz)
        std::atomic<bool> s_IsHandlingCrash = false;

        const char* GetExceptionName(DWORD code)
        {
            switch (code)
            {
            case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
            case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
            case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
            case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
            case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
            case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
            case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
            case CPP_EXCEPTION_CODE:                 return "nieobsluzony wyjatek C++";
            default:                                 return "nieznany wyjatek";
            }
        }

        void LogExceptionInfo(const EXCEPTION_RECORD& record)
        {
            LOG_CRITICAL("=== AWARIA PROGRAMU ===");
            LOG_CRITICAL("Wyjatek: {} (kod 0x{:08X})", GetExceptionName(record.ExceptionCode), record.ExceptionCode);
            LOG_CRITICAL("Adres:   0x{:X}", reinterpret_cast<std::uintptr_t>(record.ExceptionAddress));

            // Dla naruszenia dostępu system podaje rodzaj operacji i adres, do którego się odwołano
            if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record.NumberParameters >= 2)
            {
                const ULONG_PTR operation = record.ExceptionInformation[0];
                const char* operationName = operation == 0 ? "odczyt" : operation == 1 ? "zapis" : "wykonanie (DEP)";
                LOG_CRITICAL("Proba:   {} pod adresem 0x{:X}", operationName, record.ExceptionInformation[1]);
            }
        }

        // (h) Stos wywołań odtworzony z kontekstu wyjątku - pierwsza ramka to miejsce awarii
        void LogCallstack(const CONTEXT* context)
        {
            const std::vector<SStackFrame> callstack = CaptureCallstackFromContext(context);
            if (callstack.empty())
            {
                LOG_CRITICAL("Nie udalo sie odtworzyc stosu wywolan");
                return;
            }

            LOG_CRITICAL("Stos wywolan:");
            for (std::size_t i = 0; i < callstack.size(); ++i)
                LOG_CRITICAL("  #{:<2} {}", i, FormatStackFrame(callstack[i]));
        }

        // (c, d, e) Zapis zrzutu pamięci. Zwraca true, jeżeli się udało.
        bool WriteDump(EXCEPTION_POINTERS* exceptionPointers)
        {
            // (c) Katalog mógł zostać usunięty w trakcie działania gry - tworzymy go ponownie
            if (!CreateDirectoryW(s_DumpDirectory, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                LOG_CRITICAL("Nie udalo sie utworzyc katalogu zrzutow (blad {})", GetLastError());
                return false;
            }

            // (d) Unikalna nazwa: crash_2026-11-17_18-52-31.dmp (+ PID, gdyby dwa procesy padły w tej samej sekundzie)
            char timestamp[32] = "unknown-time";
            Paths::FormatFileTimestamp(timestamp, sizeof(timestamp));

            wchar_t dumpPath[MAX_PATH];
            const int written = std::swprintf(dumpPath, MAX_PATH, L"%ls\\crash_%hs_%lu.dmp",
                                              s_DumpDirectory, timestamp, GetCurrentProcessId());
            if (written < 0)
            {
                LOG_CRITICAL("Sciezka do pliku zrzutu jest za dluga");
                return false;
            }

            const std::filesystem::path dumpPathForLog(dumpPath);
            LOG_CRITICAL("Zapisywanie zrzutu pamieci: {}", dumpPathForLog.string());

            const HANDLE file = CreateFileW(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                LOG_CRITICAL("Nie udalo sie utworzyc pliku zrzutu (blad {})", GetLastError());
                return false;
            }

            // Informacja o wyjątku - Visual Studio po otwarciu .dmp od razu pokaże miejsce awarii
            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};
            exceptionInfo.ThreadId = GetCurrentThreadId();
            exceptionInfo.ExceptionPointers = exceptionPointers;
            exceptionInfo.ClientPointers = FALSE;   // wskaźniki należą do tego procesu

            // (e) Pełna pamięć procesu + dodatkowe informacje o uchwytach, wątkach i modułach.
            // Plik może mieć setki MB, ale pozwala podejrzeć dowolną zmienną w chwili awarii.
            const auto dumpType = static_cast<MINIDUMP_TYPE>(
                MiniDumpWithFullMemory |
                MiniDumpWithFullMemoryInfo |
                MiniDumpWithHandleData |
                MiniDumpWithThreadInfo |
                MiniDumpWithUnloadedModules);

            const BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, dumpType,
                                                   exceptionPointers ? &exceptionInfo : nullptr, nullptr, nullptr);
            // MiniDumpWriteDump zwraca błąd jako HRESULT przez GetLastError
            const DWORD error = success ? 0 : GetLastError();

            CloseHandle(file);

            if (!success)
            {
                LOG_CRITICAL("MiniDumpWriteDump nie powiodlo sie (HRESULT 0x{:08X})", error);
                DeleteFileW(dumpPath);  // niekompletny plik tylko by mylił
                return false;
            }

            LOG_CRITICAL("Zrzut pamieci zapisany: {}", dumpPathForLog.string());
            return true;
        }

        // (a) Funkcja wywoływana przez system dla każdego nieobsłużonego wyjątku w procesie
        LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* exceptionPointers)
        {
            // Druga awaria (np. w tym handlerze) - nie próbujemy ponownie, oddajemy systemowi
            if (s_IsHandlingCrash.exchange(true))
                return EXCEPTION_CONTINUE_SEARCH;

            // (f) Każdy etap logujemy osobno i od razu spłukujemy - jeśli coś dalej się wysypie,
            // to, co już wiemy, zostanie na dysku
            if (exceptionPointers != nullptr && exceptionPointers->ExceptionRecord != nullptr)
                LogExceptionInfo(*exceptionPointers->ExceptionRecord);
            CLogger::Get()->flush();

            if (exceptionPointers != nullptr)
                LogCallstack(exceptionPointers->ContextRecord);
            CLogger::Get()->flush();

            WriteDump(exceptionPointers);

            LOG_CRITICAL("Zamykanie procesu po awarii");
            // Proces zaraz zostanie zakończony bez wywołania destruktorów - destruktor CLogger
            // nie zdąży spłukać bufora, więc robimy to tutaj
            CLogger::Get()->flush();

            // Proces kończy się bez systemowego okna "program przestał działać"
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }

    CCrashHandler::CCrashHandler()
    {
        // (c) Katalog zrzutów tworzymy od razu, póki program działa normalnie
        std::error_code error;
        const std::filesystem::path dumpDirectory = Paths::GetCrashDumpsDirectory();
        std::filesystem::create_directories(dumpDirectory, error);
        if (error)
            LOG_WARN("Nie udalo sie utworzyc katalogu zrzutow {}: {}", dumpDirectory.string(), error.message());

        const std::wstring dumpDirectoryWide = dumpDirectory.wstring();
        if (dumpDirectoryWide.size() >= MAX_PATH)
        {
            LOG_ERROR("Sciezka katalogu zrzutow jest za dluga - crash handler wylaczony");
            return;
        }
        wcsncpy_s(s_DumpDirectory, dumpDirectoryWide.c_str(), _TRUNCATE);

        // Rezerwa stosu, z której handler skorzysta przy EXCEPTION_STACK_OVERFLOW (dotyczy tego wątku)
        ULONG stackGuarantee = STACK_GUARANTEE_BYTES;
        SetThreadStackGuarantee(&stackGuarantee);

        // Wyłącza systemowe okna dialogowe błędów - awaria ma trafić do naszego handlera
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

        // (a) Rejestracja funkcji obsługi nieobsłużonych wyjątków dla całego procesu.
        // Zastępuje też filtr CRT, który dla nieobsłużonych wyjątków C++ wołałby std::terminate.
        s_PreviousFilter = SetUnhandledExceptionFilter(OnUnhandledException);
        s_IsInstalled = true;

        LOG_INFO("Crash handler zainstalowany, zrzuty pamieci: {}", dumpDirectory.string());
    }

    CCrashHandler::~CCrashHandler()
    {
        if (!s_IsInstalled)
            return;

        SetUnhandledExceptionFilter(s_PreviousFilter);
        s_PreviousFilter = nullptr;
        s_IsInstalled = false;
    }

    bool CCrashHandler::IsSupported()
    {
        return true;
    }
}

#endif
