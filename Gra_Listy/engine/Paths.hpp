#pragma once

#include <cstddef>
#include <filesystem>

namespace Engine::Paths
{
    // Systemowy katalog na dane tworzone przez aplikacje użytkownika:
    //     Windows  %APPDATA%                      (np. C:\Users\<user>\AppData\Roaming)
    //     Linux    $XDG_DATA_HOME lub ~/.local/share
    //     macOS    ~/Library/Application Support
    // Zwraca pustą ścieżkę, jeżeli katalogu nie udało się ustalić.
    std::filesystem::path GetAppDataDirectory();

    // Katalog danych tej gry: <AppData>/ProgramowanieGier
    std::filesystem::path GetGameDataDirectory();

    // Katalog na pliki logów: <AppData>/ProgramowanieGier/logs
    std::filesystem::path GetLogsDirectory();

    // Katalog na zrzuty pamięci: <AppData>/ProgramowanieGier/crashdumps
    std::filesystem::path GetCrashDumpsDirectory();

    // Bieżąca data i godzina do nazw plików: "2026-11-17_18-52-31" (bez dwukropków -
    // niedozwolonych w nazwach plików na Windows). Zapisuje do podanego bufora, bez alokacji
    // pamięci (bezpieczne w crash handlerze). Zwraca liczbę zapisanych znaków (0 przy błędzie).
    std::size_t FormatFileTimestamp(char* buffer, std::size_t bufferSize);
}
