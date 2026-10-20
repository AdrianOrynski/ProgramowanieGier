workspace "ProgramowanieGier"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "Game"

    language "C++"
    cppdialect "C++20"
    targetdir "build/%{cfg.buildcfg}"
    objdir "build-int/%{cfg.buildcfg}/%{prj.name}"
    includedirs { "vendor/include", "." }
    libdirs { "vendor/lib" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        optimize "On"

    filter {}

-- (a) Biblioteka statyczna silnika
project "Engine"
    kind "StaticLib"
    files { "engine/**.hpp", "engine/**.cpp" }

-- (b) Aplikacja konsolowa gry, linkująca silnik
project "Game"
    kind "ConsoleApp"
    files { "game/**.hpp", "game/**.cpp" }
    links { "Engine" }

-- (c) Aplikacja konsolowa testów, linkująca silnik
project "Tests"
    kind "ConsoleApp"
    files { "tests/**.hpp", "tests/**.cpp" }
    links { "Engine" }
