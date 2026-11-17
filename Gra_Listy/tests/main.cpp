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

// --- SFrameTime (lista 2, zadanie 3) ---
#include "engine/FrameTime.hpp"

TEST_CASE("SFrameTime::Update aktualizuje wszystkie pola", "[frametime]")
{
    using Catch::Matchers::WithinAbs;

    Engine::SFrameTime time;
    REQUIRE(time.frameIndex == 0);
    REQUIRE(time.timeScale == 1.f);

    time.Update(0.5f);
    REQUIRE(time.frameIndex == 1);
    REQUIRE_THAT(time.deltaTime, WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(time.elapsedTime, WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(time.scaledDeltaTime, WithinAbs(0.5, 1e-6));
    REQUIRE_THAT(time.scaledElapsedTime, WithinAbs(0.5, 1e-6));

    SECTION("mnożnik czasu wpływa tylko na pola skalowane")
    {
        time.timeScale = 2.f;
        time.Update(0.25f);

        REQUIRE(time.frameIndex == 2);
        REQUIRE_THAT(time.deltaTime, WithinAbs(0.25, 1e-6));
        REQUIRE_THAT(time.elapsedTime, WithinAbs(0.75, 1e-6));
        REQUIRE_THAT(time.scaledDeltaTime, WithinAbs(0.5, 1e-6));
        REQUIRE_THAT(time.scaledElapsedTime, WithinAbs(1.0, 1e-6));
    }

    SECTION("pauza (timeScale = 0) zatrzymuje czas skalowany")
    {
        time.timeScale = 0.f;
        time.Update(1.f);

        REQUIRE(time.frameIndex == 2);
        REQUIRE_THAT(time.elapsedTime, WithinAbs(1.5, 1e-6));
        REQUIRE_THAT(time.scaledDeltaTime, WithinAbs(0.0, 1e-6));
        REQUIRE_THAT(time.scaledElapsedTime, WithinAbs(0.5, 1e-6));
    }
}

// --- CLogger (lista 2, zadanie 4) ---
#include "engine/Logger.hpp"

TEST_CASE("Makra LOG_* działają bez utworzonego CLogger", "[logger]")
{
    REQUIRE(Engine::CLogger::Get() != nullptr);
    REQUIRE(Engine::CLogger::GetLogFilePath().empty());

    LOG_TRACE("trace bez loggera");
    LOG_INFO("info bez loggera {}", 42);
    LOG_ERROR("error bez loggera");
}

TEST_CASE("CLogger zapisuje logi do unikalnego pliku w katalogu logów", "[logger]")
{
    std::filesystem::path logFile;
    {
        Engine::CLogger logger;
        logFile = Engine::CLogger::GetLogFilePath();

        REQUIRE_FALSE(logFile.empty());
        REQUIRE(logFile.parent_path() == Engine::Paths::GetLogsDirectory());
        REQUIRE(logFile.extension() == ".log");

        LOG_WARN("wiadomosc testowa {}", 123);
    }

    // Po zniszczeniu loggera plik jest zamknięty i spłukany
    REQUIRE(Engine::CLogger::GetLogFilePath().empty());
    REQUIRE(std::filesystem::exists(logFile));
    REQUIRE(std::filesystem::file_size(logFile) > 0);

    // Makra nadal są bezpieczne po spdlog::shutdown()
    LOG_INFO("info po zamknieciu loggera");

    std::filesystem::remove(logFile);
}

// --- ASSERT i Callstack (lista 2, zadanie 5) ---
#include "engine/Assert.hpp"
#include "engine/Callstack.hpp"

TEST_CASE("ASSERT ze spełnionym warunkiem nie przerywa programu", "[assert]")
{
    int evaluations = 0;
    auto check = [&evaluations] { ++evaluations; return true; };

    ASSERT(check(), "warunek spelniony");

#if defined(DEBUG)
    REQUIRE(evaluations == 1);  // Debug: warunek wykonany dokładnie raz
#else
    REQUIRE(evaluations == 0);  // Release: asercja znika razem z warunkiem
#endif
}

TEST_CASE("ASSERT zachowuje się jak pojedyncza instrukcja", "[assert]")
{
    // Bez do { } while (false) ten if/else by się nie skompilował albo zmienił znaczenie
    bool elseTaken = false;
    if (false)
        ASSERT(true, "nieosiagalne");
    else
        elseTaken = true;

    REQUIRE(elseTaken);
}

TEST_CASE("CaptureCallstack zwraca symbolizowane ramki", "[callstack]")
{
    const std::vector<Engine::SStackFrame> frames = Engine::CaptureCallstack();

#if defined(_WIN32)
    REQUIRE_FALSE(frames.empty());
    for (const Engine::SStackFrame& frame : frames)
    {
        CHECK(frame.address != 0);
        CHECK_FALSE(Engine::FormatStackFrame(frame).empty());
    }
  #if defined(DEBUG)
    // W Debug są symbole (.pdb) - pierwsza ramka to ta funkcja testowa, z plikiem i linią
    CHECK_FALSE(frames.front().function.empty());
    CHECK(frames.front().file.find("main.cpp") != std::string::npos);
  #endif
#else
    REQUIRE(frames.empty());
#endif
}

// --- Crash dump: znacznik czasu, katalog, stos z kontekstu (lista 2, zadanie 8) ---
#include <cstring>

#include "engine/CrashHandler.hpp"

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>    // RtlCaptureContext
#endif

TEST_CASE("Paths::FormatFileTimestamp tworzy nazwę bez niedozwolonych znaków", "[paths][crash]")
{
    char buffer[32];
    const std::size_t length = Engine::Paths::FormatFileTimestamp(buffer, sizeof(buffer));

    REQUIRE(length == std::strlen("2026-11-17_18-52-31"));
    REQUIRE(std::string(buffer).find_first_of(":\\/*?\"<>|") == std::string::npos);
}

TEST_CASE("Katalog zrzutów leży w katalogu danych gry", "[paths][crash]")
{
    REQUIRE(Engine::Paths::GetCrashDumpsDirectory().parent_path() == Engine::Paths::GetGameDataDirectory());
}

TEST_CASE("CCrashHandler instaluje się i tworzy katalog zrzutów", "[crash]")
{
    {
        Engine::CCrashHandler crashHandler;
    }

#if defined(_WIN32)
    REQUIRE(Engine::CCrashHandler::IsSupported());
    REQUIRE(std::filesystem::is_directory(Engine::Paths::GetCrashDumpsDirectory()));
#else
    REQUIRE_FALSE(Engine::CCrashHandler::IsSupported());
#endif
}

TEST_CASE("CaptureCallstackFromContext odtwarza stos z kontekstu procesora", "[callstack][crash]")
{
    REQUIRE(Engine::CaptureCallstackFromContext(nullptr).empty());

#if defined(_WIN32)
    CONTEXT context{};
    RtlCaptureContext(&context);

    const std::vector<Engine::SStackFrame> frames = Engine::CaptureCallstackFromContext(&context);
    REQUIRE_FALSE(frames.empty());

  #if defined(DEBUG)
    // Kontekst przechwycony w tej funkcji - pierwsza ramka to ona sama
    CHECK(frames.front().file.find("main.cpp") != std::string::npos);
  #endif
#endif
}
