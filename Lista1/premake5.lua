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

    -- fmt wbudowany w spdlog wymaga kompilacji źródeł jako UTF-8
    buildoptions { "/utf-8" }

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

    -- SFML 3.1 linkowany dynamicznie (DLL w vendor/bin)
    -- LNK4006: duplikaty __NULL_IMPORT_DESCRIPTOR przy scalaniu import-libów, nieszkodliwe
    linkoptions { "/IGNORE:4006" }

    -- Engine buduje się przed Game i Tests, więc tu kopiujemy DLL-ki do wspólnego targetdir
    postbuildcommands { '{COPYFILE} "%{wks.location}vendor/bin/*.dll" "%{cfg.targetdir}"' }

    filter "configurations:Debug"
        links { "sfml-graphics-d", "sfml-window-d", "sfml-system-d" }

    filter "configurations:Release"
        links { "sfml-graphics", "sfml-window", "sfml-system" }

    filter {}

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
