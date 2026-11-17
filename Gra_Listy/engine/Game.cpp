#include "engine/Game.hpp"

#include <spdlog/spdlog.h>

#include <imgui/imgui.h>
#include <imgui/SFML/imgui-SFML.h>

namespace Engine
{
    CGame::CGame(unsigned int width, unsigned int height, const std::string& title)
        : m_window(sf::VideoMode({ width, height }), title)
        , m_shape(100.f)
    {
        m_window.setFramerateLimit(60);

        m_imguiInitialized = ImGui::SFML::Init(m_window);
        if (!m_imguiInitialized)
        {
            spdlog::error("Nie udalo sie zainicjalizowac ImGui-SFML");
            m_window.close();
        }

        m_shape.setFillColor(sf::Color::Green);
        m_shape.setPosition({ 300.f, 200.f });
    }

    CGame::~CGame()
    {
        if (m_imguiInitialized)
            ImGui::SFML::Shutdown();
    }

    void CGame::Run()
    {
        m_deltaClock.restart();

        while (m_window.isOpen())
        {
            Tick();
            Render();
        }
    }

    void CGame::Tick()
    {
        const sf::Time deltaTime = m_deltaClock.restart();

        ProcessEvents();
        Update(deltaTime);
    }

    void CGame::ProcessEvents()
    {
        while (const std::optional event = m_window.pollEvent())
            ProcessEvent(*event);
    }

    void CGame::ProcessEvent(const sf::Event& event)
    {
        ImGui::SFML::ProcessEvent(m_window, event);

        if (event.is<sf::Event::Closed>())
            m_window.close();
    }

    void CGame::Update(sf::Time deltaTime)
    {
        ImGui::SFML::Update(m_window, deltaTime);

        // Okno demo ImGui - sprawdzenie, czy imgui i imgui-sfml działają
        ImGui::ShowDemoWindow();
    }

    void CGame::Render()
    {
        // Po zamknięciu okna w ProcessEvent nie ma już do czego rysować
        if (!m_window.isOpen())
            return;

        m_window.clear();
        m_window.draw(m_shape);
        ImGui::SFML::Render(m_window);
        m_window.display();
    }
}
