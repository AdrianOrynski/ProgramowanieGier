#pragma once

#include <string>

#include <SFML/Graphics.hpp>

#include "engine/FrameTime.hpp"
#include "engine/NonCopyable.hpp"

namespace Engine
{
    // Główna klasa gry - okno SFML, ImGui i pętla gry
    class CGame : private NonCopyable
    {
    public:
        CGame(unsigned int width = 800, unsigned int height = 600, const std::string& title = "Game");
        ~CGame();

        // Nieskończona pętla gry - kończy się po zamknięciu okna
        void Run();

        const SFrameTime& GetFrameTime() const { return m_frameTime; }

    private:
        // Jedna klatka: aktualizacja SFrameTime + eventy SFML + aktualizacja
        void Tick();

        void ProcessEvents();
        void ProcessEvent(const sf::Event& event);
        void Update();
        void Render();

        // Okno ImGui z podglądem SFrameTime i suwakiem mnożnika czasu
        void DrawFrameTimeWindow();

        sf::RenderWindow m_window;
        sf::Clock m_deltaClock;
        SFrameTime m_frameTime;
        bool m_imguiInitialized = false;

        // Zastępczy obiekt sceny, dopóki nie ma prawdziwej zawartości gry
        sf::CircleShape m_shape;
    };
}
