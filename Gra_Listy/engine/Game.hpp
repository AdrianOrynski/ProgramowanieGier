#pragma once

#include <string>

#include <SFML/Graphics.hpp>

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

    private:
        // Jedna klatka: czas trwania ostatniej klatki + eventy SFML + aktualizacja
        void Tick();

        void ProcessEvents();
        void ProcessEvent(const sf::Event& event);
        void Update(sf::Time deltaTime);
        void Render();

        sf::RenderWindow m_window;
        sf::Clock m_deltaClock;
        bool m_imguiInitialized = false;

        // Zastępczy obiekt sceny, dopóki nie ma prawdziwej zawartości gry
        sf::CircleShape m_shape;
    };
}
