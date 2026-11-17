#include "engine/Logger.hpp"

#include <chrono>
#include <ctime>
#include <string>
#include <system_error>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "engine/Paths.hpp"

namespace Engine
{
    namespace
    {
        constexpr const char* LOGGER_NAME = "uwr_logger";

        // Data z dokładnością do milisekund, poziom (kolorowany w konsoli), plik:linia, wiadomość:
        // [2026-11-17 18:52:31.153] [info] [Game.cpp:42] Player spawned
        constexpr const char* LOG_PATTERN = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v";

        // Unikalna nazwa pliku dla każdego uruchomienia: 2026-11-17_18-52-31.log
        // (dwukropki są niedozwolone w nazwach plików na Windows)
        std::string MakeLogFileName()
        {
            const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            std::tm local{};
#if defined(_WIN32)
            localtime_s(&local, &now);
#else
            localtime_r(&now, &local);
#endif
            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S.log", &local);
            return buffer;
        }
    }

    CLogger::CLogger()
    {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        // Plik logu w katalogu użytkownika. Jeżeli się nie uda, logujemy dalej przynajmniej do konsoli.
        std::string fileError;
        try
        {
            const std::filesystem::path logsDirectory = Paths::GetLogsDirectory();
            std::filesystem::create_directories(logsDirectory);

            s_LogFilePath = logsDirectory / MakeLogFileName();
            // truncate = false: gdyby dwa uruchomienia trafiły w tę samą sekundę, dopisujemy zamiast nadpisywać
            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(s_LogFilePath.string(), false));
        }
        catch (const std::exception& e)
        {
            fileError = e.what();
            s_LogFilePath.clear();
        }

        s_Logger = std::make_shared<spdlog::logger>(LOGGER_NAME, sinks.begin(), sinks.end());
        s_Logger->set_pattern(LOG_PATTERN);

#if defined(DEBUG)
        s_Logger->set_level(spdlog::level::trace);
#else
        s_Logger->set_level(spdlog::level::info);
#endif
        // Ostrzeżenia i błędy trafiają na dysk od razu - nie zginą, jeśli program się wywali
        s_Logger->flush_on(spdlog::level::warn);

        spdlog::register_logger(s_Logger);
        spdlog::set_default_logger(s_Logger);

        if (fileError.empty())
            LOG_INFO("Logger zainicjalizowany, plik logu: {}", s_LogFilePath.string());
        else
            LOG_ERROR("Nie udalo sie utworzyc pliku logu ({}), loguje tylko do konsoli", fileError);
    }

    CLogger::~CLogger()
    {
        LOG_INFO("Zamykanie loggera");

        s_Logger->flush();
        s_Logger.reset();
        s_LogFilePath.clear();

        // Usuwa zarejestrowane loggery i zamyka pliki; kolejne LOG_* trafią do nowego domyślnego loggera
        spdlog::shutdown();
    }

    spdlog::logger* CLogger::Get()
    {
        if (s_Logger)
            return s_Logger.get();

        // Przed utworzeniem CLogger działa domyślny logger spdlog
        if (spdlog::logger* defaultLogger = spdlog::default_logger_raw())
            return defaultLogger;

        // Po spdlog::shutdown() domyślnego loggera już nie ma (registry::drop_all go zeruje),
        // a jakiś destruktor statycznego obiektu może jeszcze chcieć coś zalogować
        static const std::shared_ptr<spdlog::logger> fallback = []
        {
            auto logger = std::make_shared<spdlog::logger>(
                "uwr_fallback", std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            logger->set_pattern(LOG_PATTERN);
            return logger;
        }();
        return fallback.get();
    }

    const std::filesystem::path& CLogger::GetLogFilePath()
    {
        return s_LogFilePath;
    }
}
