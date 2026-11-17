#include "engine/Game.hpp"

#include "engine/Assert.hpp"
#include "engine/Logger.hpp"
#include "engine/WindowEvents.hpp"

#include <imgui/imgui.h>
#include <imgui/SFML/imgui-SFML.h>

namespace Engine
{
    namespace
    {
        // Wydarzenie demonstracyjne do sprawdzania kolejki (okno ImGui "Events")
        struct SDemoDelayedEvent : TEvent<SDemoDelayedEvent>
        {
            SDemoDelayedEvent(std::string description, float queuedAt)
                : description(std::move(description))
                , queuedAt(queuedAt)
            {
            }

            std::string description;
            float queuedAt;
        };
    }

    CGame::CGame(unsigned int width, unsigned int height, const std::string& title)
        : m_window(sf::VideoMode({ width, height }), title)
        , m_shape(100.f)
    {
        m_window.setFramerateLimit(60);

        m_imguiInitialized = ImGui::SFML::Init(m_window);
        if (!m_imguiInitialized)
        {
            LOG_ERROR("Nie udalo sie zainicjalizowac ImGui-SFML");
            m_window.close();
        }

        m_shape.setFillColor(sf::Color::Green);
        m_shape.setPosition({ 300.f, 200.f });

        SubscribeToEvents();

        LOG_INFO("CGame utworzony, okno {}x{} \"{}\"", width, height, title);
    }

    CGame::~CGame()
    {
        LOG_INFO("CGame niszczony po {} klatkach ({:.2f} s)", m_frameTime.frameIndex, m_frameTime.elapsedTime);

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
        m_eventDispatcher->Frame(m_frameTime);
        Update();
    }

    void CGame::SubscribeToEvents()
    {
        // (j) Zamknięcie okna przez wewnętrzne wydarzenie zamiast bezpośrednio sf::Event::Closed
        m_eventHandles.push_back(m_eventDispatcher->Subscribe<SWindowClosedEvent>(
            [this](const SWindowClosedEvent&)
            {
                LOG_INFO("SWindowClosedEvent - zamykanie okna");
                m_window.close();
            }));

        // Widok podąża za rozmiarem okna, żeby obraz się nie rozciągał
        m_eventHandles.push_back(m_eventDispatcher->Subscribe<SWindowResizedEvent>(
            [this](const SWindowResizedEvent& event)
            {
                LOG_DEBUG("SWindowResizedEvent - {}x{}", event.size.x, event.size.y);
                m_window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, sf::Vector2f(event.size))));
            }));

        m_eventHandles.push_back(m_eventDispatcher->Subscribe<SKeyPressedEvent>(
            [](const SKeyPressedEvent& event)
            {
                LOG_DEBUG("SKeyPressedEvent - {}", sf::Keyboard::getDescription(event.scancode).toAnsiString());
            }));

        // Kliknięcie przesuwa kółko (o ile kursor nie jest nad oknem ImGui)
        m_eventHandles.push_back(m_eventDispatcher->Subscribe<SMouseButtonPressedEvent>(
            [this](const SMouseButtonPressedEvent& event)
            {
                if (event.button != sf::Mouse::Button::Left || ImGui::GetIO().WantCaptureMouse)
                    return;

                const sf::Vector2f worldPosition = m_window.mapPixelToCoords(event.position);
                m_shape.setPosition(worldPosition - sf::Vector2f(m_shape.getRadius(), m_shape.getRadius()));
            }));

        m_eventHandles.push_back(m_eventDispatcher->Subscribe<SDemoDelayedEvent>(
            [this](const SDemoDelayedEvent& event)
            {
                LOG_INFO("SDemoDelayedEvent '{}' - zakolejkowane w {:.2f} s, odebrane w {:.2f} s (klatka {})",
                         event.description, event.queuedAt, m_frameTime.elapsedTime, m_frameTime.frameIndex);
            }));
    }

    void CGame::ProcessEvents()
    {
        while (const std::optional event = m_window.pollEvent())
            ProcessEvent(*event);
    }

    void CGame::ProcessEvent(const sf::Event& event)
    {
        ImGui::SFML::ProcessEvent(m_window, event);

        CEventDispatcher& dispatcher = *m_eventDispatcher;

        if (event.is<sf::Event::Closed>())
            dispatcher.Submit<SWindowClosedEvent>();
        else if (const auto* resized = event.getIf<sf::Event::Resized>())
            dispatcher.Submit<SWindowResizedEvent>(*resized);
        else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
            dispatcher.Submit<SKeyPressedEvent>(*keyPressed);
        else if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>())
            dispatcher.Submit<SKeyReleasedEvent>(*keyReleased);
        else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
            dispatcher.Submit<SMouseButtonPressedEvent>(*mousePressed);
        else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
            dispatcher.Submit<SMouseMovedEvent>(*mouseMoved);
        else if (const auto* wheelScrolled = event.getIf<sf::Event::MouseWheelScrolled>())
            dispatcher.Submit<SMouseWheelScrolledEvent>(*wheelScrolled);
    }

    void CGame::Update()
    {
        ASSERT(m_frameTime.deltaTime >= 0.f, "Czas klatki nie moze byc ujemny");

        // ImGui dostaje czas rzeczywisty - interfejs nie powinien zwalniać razem z grą
        ImGui::SFML::Update(m_window, sf::seconds(m_frameTime.deltaTime));

        // Okno demo ImGui - sprawdzenie, czy imgui i imgui-sfml działają
        ImGui::ShowDemoWindow();

        DrawFrameTimeWindow();
        DrawEventsWindow();
    }

    void CGame::DrawEventsWindow()
    {
        ImGui::Begin("Events");

        ImGui::Text("Sluchacze:               %zu", m_eventDispatcher->GetListenerCount());
        ImGui::Text("Zakolejkowane wydarzenia: %zu", m_eventDispatcher->GetQueuedEventCount());
        ImGui::Separator();

        // Opóźnienie czasowe liczone w czasie skalowanym - przy Time scale = 0 wydarzenie czeka
        if (ImGui::Button("Wydarzenie za 2 s"))
            m_eventDispatcher->QueueWithTimeDelay<SDemoDelayedEvent>(2.f, "2 s", m_frameTime.elapsedTime);

        if (ImGui::Button("Wydarzenie za 120 klatek"))
            m_eventDispatcher->QueueWithFrameDelay<SDemoDelayedEvent>(120, "120 klatek", m_frameTime.elapsedTime);

        ImGui::End();
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

#if defined(DEBUG)
        // Ręczne sprawdzenie systemu asercji: log + stos wywołań, breakpoint pod debuggerem, abort
        ImGui::Separator();
        if (ImGui::Button("Wywolaj ASSERT"))
            ASSERT(m_frameTime.frameIndex == 0, "Testowa asercja wywolana z okna ImGui");
#endif

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
