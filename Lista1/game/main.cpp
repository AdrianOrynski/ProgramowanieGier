#include <iostream>

#include "engine/Engine.hpp"

int main()
{
    std::cout << "Game linked with " << Engine::GetName() << "\n";
    return 0;
}
