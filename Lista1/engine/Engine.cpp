#include "engine/Engine.hpp"

#include <SFML/Graphics.hpp>

#include <imgui/imgui.h>
#include <imgui/SFML/imgui-SFML.h>

namespace Engine
{
    const char* GetName()
    {
        return "Engine";
    }

    void RunSfmlExample()
    {
        sf::RenderWindow window(sf::VideoMode({ 800, 600 }), "SFML window");
        window.setFramerateLimit(60);

        if (!ImGui::SFML::Init(window))
            return;

        // Zamiast tekstury/fontu/muzyki z przykładu (brak plików) - zwykły kształt
        sf::CircleShape shape(100.f);
        shape.setFillColor(sf::Color::Green);
        shape.setPosition({ 300.f, 200.f });

        sf::Clock deltaClock;
        while (window.isOpen())
        {
            while (const std::optional event = window.pollEvent())
            {
                ImGui::SFML::ProcessEvent(window, *event);

                if (event->is<sf::Event::Closed>())
                    window.close();
            }

            ImGui::SFML::Update(window, deltaClock.restart());

            // Okno demo ImGui - sprawdzenie, czy imgui i imgui-sfml działają
            ImGui::ShowDemoWindow();

            window.clear();
            window.draw(shape);
            ImGui::SFML::Render(window);
            window.display();
        }

        ImGui::SFML::Shutdown();
    }
}
