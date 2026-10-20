#include <iostream>

#include <nlohmann/json.hpp>

#include "engine/Engine.hpp"

int main()
{
    std::cout << "Game linked with " << Engine::GetName() << "\n";

    // Sprawdzenie, czy nlohmann/json jest widoczny
    nlohmann::json config = { { "engine", Engine::GetName() }, { "width", 800 }, { "height", 600 } };
    std::cout << "JSON: " << config.dump() << "\n";

    Engine::RunSfmlExample();
    return 0;
}
