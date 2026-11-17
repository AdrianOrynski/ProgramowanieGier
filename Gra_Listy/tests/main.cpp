#include <catch_amalgamated.hpp>

#include "engine/Engine.hpp"

// Własny main zamiast tego z catch_amalgamated.cpp (CATCH_AMALGAMATED_CUSTOM_MAIN w premake5.lua)
int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}

TEST_CASE("Engine zwraca swoją nazwę", "[engine]")
{
    REQUIRE(std::string(Engine::GetName()) == "Engine");
}
