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
        m_frameTime.Update(m_deltaClock.restart().asSeconds());

        ProcessEvents();
        Update();
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

    void CGame::Update()
    {
        // ImGui dostaje czas rzeczywisty - interfejs nie powinien zwalniać razem z grą
        ImGui::SFML::Update(m_window, sf::seconds(m_frameTime.deltaTime));

        // Okno demo ImGui - sprawdzenie, czy imgui i imgui-sfml działają
        ImGui::ShowDemoWindow();

        DrawFrameTimeWindow();
    }

    void CGame::DrawFrameTimeWindow()
    {
        ImGui::Begin("Frame time");

        ImGui::Text("Frame index:          %llu", static_cast<unsigned long long>(m_frameTime.frameIndex));
        ImGui::Text("Delta time:           %.4f s (%.1f FPS)", m_frameTime.deltaTime,
                    m_frameTime.deltaTime > 0.f ? 1.f / m_frameTime.deltaTime : 0.f);
        ImGui::Text("Elapsed time:         %.2f s", m_frameTime.elapsedTime);
        ImGui::Separator();
        ImGui::Text("Scaled delta time:    %.4f s", m_frameTime.scaledDeltaTime);
        ImGui::Text("Scaled elapsed time:  %.2f s", m_frameTime.scaledElapsedTime);
        ImGui::SliderFloat("Time scale", &m_frameTime.timeScale, 0.f, 4.f);

        ImGui::End();
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
