#pragma once

#include "engine/NonCopyable.hpp"

namespace Engine
{
    // Przechwytuje każdy nieobsłużony wyjątek w procesie i zapisuje zrzut pamięci (.dmp)
    // do <AppData>/ProgramowanieGier/crashdumps/crash_<data>_<godzina>.dmp.
    // Informacje o awarii (kod i adres wyjątku, stos wywołań, ścieżka zrzutu, błędy zapisu)
    // trafiają do loggera.
    //
    // RAII, tak jak CLogger: tworzymy zaraz PO loggerze, przed pozostałymi systemami gry.
    // Destruktor przywraca poprzednią funkcję obsługi.
    //
    //     int main()
    //     {
    //         Engine::CLogger logger;
    //         Engine::CCrashHandler crashHandler;
    //         Engine::CGame game;
    //         ...
    //     }
    //
    // Implementacja zależna od platformy jest w osobnych plikach:
    //     CrashHandlerWindows.cpp - SetUnhandledExceptionFilter + DbgHelp (MiniDumpWriteDump)
    //     CrashHandlerNull.cpp    - pozostałe platformy, nic nie robi
    //
    // Uwaga: pod debuggerem (F5) wyjątek przechwytuje debugger i handler nie jest wywoływany.
    // Żeby zobaczyć zrzut, uruchom grę bez debuggera (Ctrl+F5).
    class CCrashHandler : private NonCopyable
    {
    public:
        CCrashHandler();
        ~CCrashHandler();

        // Czy na tej platformie handler faktycznie działa
        static bool IsSupported();
    };
}
