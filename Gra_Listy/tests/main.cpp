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

// --- NonCopyable (lista 2, zadanie 1) ---
#include <type_traits>

#include "engine/NonCopyable.hpp"

namespace
{
    // Dziedziczenie: kopiowanie usunięte, przenoszenie nadal możliwe
    class CInherited : private Engine::NonCopyable
    {
    };

    // Makro w ciele klasy: kopiowanie usunięte, przenoszenie zdefiniowane ręcznie
    class CWithMacro
    {
        NON_COPYABLE(CWithMacro);

    public:
        CWithMacro() = default;
        CWithMacro(CWithMacro&&) noexcept = default;
        CWithMacro& operator=(CWithMacro&&) noexcept = default;
    };
}

TEST_CASE("NonCopyable usuwa kopiowanie i zostawia przenoszenie", "[noncopyable]")
{
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<CInherited>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<CInherited>);
    STATIC_REQUIRE(std::is_move_constructible_v<CInherited>);
    STATIC_REQUIRE(std::is_move_assignable_v<CInherited>);

    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<CWithMacro>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<CWithMacro>);
    STATIC_REQUIRE(std::is_move_constructible_v<CWithMacro>);
    STATIC_REQUIRE(std::is_move_assignable_v<CWithMacro>);
}

// --- Paths (lista 2, zadanie 2) ---
#include <filesystem>

#include "engine/Paths.hpp"

TEST_CASE("Paths::GetAppDataDirectory zwraca istniejący katalog", "[paths]")
{
    const std::filesystem::path appData = Engine::Paths::GetAppDataDirectory();

    REQUIRE_FALSE(appData.empty());
    REQUIRE(appData.is_absolute());
    REQUIRE(std::filesystem::is_directory(appData));
}
