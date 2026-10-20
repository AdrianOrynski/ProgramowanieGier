#include <iostream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "engine/Engine.hpp"

int main()
{
    std::cout << "Game linked with " << Engine::GetName() << "\n";

    // Sprawdzenie, czy nlohmann/json jest widoczny
    nlohmann::json config = { { "engine", Engine::GetName() }, { "width", 800 }, { "height", 600 } };
    std::cout << "JSON: " << config.dump() << "\n";

    // Sprawdzenie, czy spdlog jest widoczny
    spdlog::info("spdlog {}.{}.{} dziala", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

    Engine::RunSfmlExample();
    return 0;
}
