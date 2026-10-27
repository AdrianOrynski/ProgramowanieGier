#pragma once

#include <string>

#include <SFML/Graphics.hpp>

namespace Engine
{
    // Główna klasa gry - okno SFML, ImGui i pętla gry
    class CGame
    {
    public:
        CGame(unsigned int width = 800, unsigned int height = 600, const std::string& title = "Game");
        ~CGame();

        CGame(const CGame&) = delete;
        CGame& operator=(const CGame&) = delete;

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
