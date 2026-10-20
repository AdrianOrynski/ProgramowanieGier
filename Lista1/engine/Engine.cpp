#include "engine/Engine.hpp"

#include <SFML/Graphics.hpp>

namespace Engine
{
    const char* GetName()
    {
        return "Engine";
    }

    void RunSfmlExample()
    {
        sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "SFML window");

        // Zamiast tekstury/fontu/muzyki z przykładu (brak plików) - zwykły kształt
        sf::CircleShape shape(100.f);
        shape.setFillColor(sf::Color::Green);
        shape.setPosition({ 300.f, 200.f });

        while (window.isOpen())
        {
            while (const std::optional event = window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    window.close();
            }

            window.clear();
            window.draw(shape);
            window.display();
        }
    }
}
