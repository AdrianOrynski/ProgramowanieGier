#include "engine/Paths.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <ShlObj.h>     // SHGetKnownFolderPath, FOLDERID_RoamingAppData

    // SHGetKnownFolderPath jest w shell32, CoTaskMemFree w ole32
    #pragma comment(lib, "shell32.lib")
    #pragma comment(lib, "ole32.lib")
#endif

namespace Engine::Paths
{
    namespace
    {
        // Zmienna środowiskowa jako ścieżka, pusta jeżeli nie istnieje
        std::filesystem::path GetEnvPath(const char* name)
        {
#if defined(_WIN32)
            // getenv jest na MSVC oznaczony jako "unsafe" (C4996)
            char* value = nullptr;
            std::size_t length = 0;
            if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
                return {};

            std::filesystem::path result = value;
            std::free(value);
            return result;
#else
            const char* value = std::getenv(name);
            return value != nullptr ? std::filesystem::path(value) : std::filesystem::path();
#endif
        }
    }

    std::filesystem::path GetAppDataDirectory()
    {
#if defined(_WIN32)
        // Oficjalne API zamiast samego %APPDATA% - zmienną środowiskową można nadpisać
        // lub usunąć, a known folder zawsze wskazuje faktyczny katalog profilu.
        // Ścieżka jest w UTF-16 (PWSTR), więc nazwy z polskimi znakami nie psują się.
        PWSTR widePath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &widePath)))
        {
            std::filesystem::path result = widePath;
            CoTaskMemFree(widePath);
            return result;
        }
        CoTaskMemFree(widePath); // wymagane nawet przy błędzie

        return GetEnvPath("APPDATA");

#elif defined(__APPLE__)
        const std::filesystem::path home = GetEnvPath("HOME");
        return home.empty() ? home : home / "Library" / "Application Support";

#else
        // Specyfikacja XDG Base Directory: $XDG_DATA_HOME, domyślnie ~/.local/share
        if (std::filesystem::path xdg = GetEnvPath("XDG_DATA_HOME"); !xdg.empty())
            return xdg;

        const std::filesystem::path home = GetEnvPath("HOME");
        return home.empty() ? home : home / ".local" / "share";
#endif
    }

    std::filesystem::path GetGameDataDirectory()
    {
        return GetAppDataDirectory() / "ProgramowanieGier";
    }

    std::filesystem::path GetLogsDirectory()
    {
        return GetGameDataDirectory() / "logs";
    }

    std::filesystem::path GetCrashDumpsDirectory()
    {
        return GetGameDataDirectory() / "crashdumps";
    }

    std::size_t FormatFileTimestamp(char* buffer, std::size_t bufferSize)
    {
        const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::tm local{};
#if defined(_WIN32)
        if (localtime_s(&local, &now) != 0)
            return 0;
#else
        if (localtime_r(&now, &local) == nullptr)
            return 0;
#endif
        return std::strftime(buffer, bufferSize, "%Y-%m-%d_%H-%M-%S", &local);
    }
}
