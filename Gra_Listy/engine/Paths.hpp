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
}
