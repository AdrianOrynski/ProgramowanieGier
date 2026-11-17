#include <iostream>

#include <nlohmann/json.hpp>

#include "engine/CrashHandler.hpp"
#include "engine/Engine.hpp"
#include "engine/Game.hpp"
#include "engine/Logger.hpp"
#include "engine/Paths.hpp"

int main()
{
    // Logger jako pierwszy obiekt w main: inicjalizuje się przed wszystkimi systemami
    // i niszczy jako ostatni (obiekty lokalne są niszczone w odwrotnej kolejności)
    Engine::CLogger logger;

    // Crash handler zaraz po loggerze (loguje przez niego), przed pozostałymi systemami gry
    Engine::CCrashHandler crashHandler;

    std::cout << "Game linked with " << Engine::GetName() << "\n";

    // Sprawdzenie, czy nlohmann/json jest widoczny
    nlohmann::json config = { { "engine", Engine::GetName() }, { "width", 800 }, { "height", 600 } };
    std::cout << "JSON: " << config.dump() << "\n";

    // Sprawdzenie, czy spdlog jest widoczny
    LOG_INFO("spdlog {}.{}.{} dziala", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

    LOG_INFO("Katalog AppData: {}", Engine::Paths::GetAppDataDirectory().string());

    Engine::CGame game(800, 600, "Game");
    game.Run();
    return 0;
}
