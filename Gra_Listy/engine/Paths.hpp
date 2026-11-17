#pragma once

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
}
