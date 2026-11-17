#pragma once

#include <filesystem>
#include <memory>

#include <spdlog/spdlog.h>

#include "engine/NonCopyable.hpp"

namespace Engine
{
    // Logger silnika: jeden spdlog::logger z dwoma sinkami - kolorowa konsola i plik
    // <AppData>/ProgramowanieGier/logs/<data>_<godzina>.log
    //
    // Cykl życia (RAII): obiekt CLogger tworzymy jako PIERWSZY w main(), przed wszystkimi
    // innymi systemami. Jako pierwszy utworzony zostanie zniszczony jako ostatni, a jego
    // destruktor spłukuje bufory i zamyka spdlog.
    //
    //     int main()
    //     {
    //         Engine::CLogger logger;
    //         Engine::CGame game;
    //         ...
    //     }
    class CLogger : private NonCopyable
    {
    public:
        CLogger();
        ~CLogger();

        // Logger używany przez makra LOG_*. Przed utworzeniem CLogger (np. w testach)
        // i po jego zniszczeniu zwraca domyślny logger spdlog (konsola), więc makra są
        // bezpieczne do wywołania w dowolnym momencie.
        static spdlog::logger* Get();

        // Ścieżka do pliku logu bieżącego uruchomienia (pusta, gdy logger nie działa)
        static const std::filesystem::path& GetLogFilePath();

    private:
        static inline std::shared_ptr<spdlog::logger> s_Logger;
        static inline std::filesystem::path s_LogFilePath;
    };
}

// Logowanie z dowolnego miejsca w kodzie, z formatowaniem fmt:
//     LOG_INFO("Gracz {} dostal {} obrazen", name, damage);
//
// Makra przekazują do spdlog miejsce wywołania (plik, linia, funkcja), które trafia do logu.
// Nie używamy SPDLOG_TRACE/SPDLOG_DEBUG, bo te są wycinane w czasie kompilacji przez
// SPDLOG_ACTIVE_LEVEL (domyślnie info) - filtrowaniem zajmuje się poziom loggera.
#define LOG_AT_LEVEL(level, ...) \
    ::Engine::CLogger::Get()->log(::spdlog::source_loc{ __FILE__, __LINE__, SPDLOG_FUNCTION }, level, __VA_ARGS__)

#define LOG_TRACE(...)    LOG_AT_LEVEL(::spdlog::level::trace, __VA_ARGS__)
#define LOG_DEBUG(...)    LOG_AT_LEVEL(::spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...)     LOG_AT_LEVEL(::spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)     LOG_AT_LEVEL(::spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...)    LOG_AT_LEVEL(::spdlog::level::err, __VA_ARGS__)
#define LOG_CRITICAL(...) LOG_AT_LEVEL(::spdlog::level::critical, __VA_ARGS__)
