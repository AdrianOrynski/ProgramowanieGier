#pragma once

#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "engine/EventDispatcher.hpp"
#include "engine/EventHandle.hpp"
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
        CEventDispatcher& GetEventDispatcher() { return *m_eventDispatcher; }

    private:
        // Jedna klatka: aktualizacja SFrameTime + eventy SFML + kolejka wydarzeń + aktualizacja
        void Tick();

        // Reakcje gry na wydarzenia (zamknięcie okna, zmiana rozmiaru, wejście)
        void SubscribeToEvents();

        void ProcessEvents();
        // Tłumaczy natywne sf::Event na własne wydarzenia i wysyła je przez dispatcher
        void ProcessEvent(const sf::Event& event);
        void Update();
        void Render();

        // Okno ImGui z podglądem SFrameTime i suwakiem mnożnika czasu
        void DrawFrameTimeWindow();
        // Okno ImGui do ręcznego sprawdzenia kolejkowania wydarzeń
        void DrawEventsWindow();

        sf::RenderWindow m_window;
        sf::Clock m_deltaClock;
        SFrameTime m_frameTime;
        bool m_imguiInitialized = false;

        // shared_ptr, bo uchwyty trzymają do dispatchera weak_ptr.
        // Zadeklarowany przed uchwytami - uchwyty niszczą się pierwsze i wypisują słuchaczy.
        std::shared_ptr<CEventDispatcher> m_eventDispatcher = std::make_shared<CEventDispatcher>();
        std::vector<CEventHandle> m_eventHandles;

        // Zastępczy obiekt sceny, dopóki nie ma prawdziwej zawartości gry
        sf::CircleShape m_shape;
    };
}
