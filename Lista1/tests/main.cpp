#include <iostream>

#include "engine/Engine.hpp"

int main()
{
    std::cout << "Tests linked with " << Engine::GetName() << "\n";
    return 0;
}
