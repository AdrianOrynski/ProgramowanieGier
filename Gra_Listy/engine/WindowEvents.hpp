#pragma once

#include <SFML/Window/Event.hpp>

#include "engine/Event.hpp"

namespace Engine
{
    // (i) Natywne wydarzenia okna i urządzeń wejściowych. Każde przenosi dane z odpowiedniego
    // podtypu sf::Event, dzięki czemu reszta gry nie musi zależeć od sf::Event.
    // Konstruktory przyjmują podtyp sf::Event - pasują do CEventDispatcher::Submit<T>(args...).

    struct SWindowClosedEvent : TEvent<SWindowClosedEvent>
    {
        SWindowClosedEvent() = default;
    };

    struct SWindowResizedEvent : TEvent<SWindowResizedEvent>
    {
        explicit SWindowResizedEvent(const sf::Event::Resized& event)
            : size(event.size)
        {
        }

        sf::Vector2u size;
    };

    struct SKeyPressedEvent : TEvent<SKeyPressedEvent>
    {
        explicit SKeyPressedEvent(const sf::Event::KeyPressed& event)
            : code(event.code)
            , scancode(event.scancode)
            , alt(event.alt)
            , control(event.control)
            , shift(event.shift)
            , system(event.system)
        {
        }

        sf::Keyboard::Key code;
        sf::Keyboard::Scancode scancode;
        bool alt;
        bool control;
        bool shift;
        bool system;
    };

    struct SKeyReleasedEvent : TEvent<SKeyReleasedEvent>
    {
        explicit SKeyReleasedEvent(const sf::Event::KeyReleased& event)
            : code(event.code)
            , scancode(event.scancode)
            , alt(event.alt)
            , control(event.control)
            , shift(event.shift)
            , system(event.system)
        {
        }

        sf::Keyboard::Key code;
        sf::Keyboard::Scancode scancode;
        bool alt;
        bool control;
        bool shift;
        bool system;
    };

    struct SMouseButtonPressedEvent : TEvent<SMouseButtonPressedEvent>
    {
        explicit SMouseButtonPressedEvent(const sf::Event::MouseButtonPressed& event)
            : button(event.button)
            , position(event.position)
        {
        }

        sf::Mouse::Button button;
        sf::Vector2i position;
    };

    struct SMouseMovedEvent : TEvent<SMouseMovedEvent>
    {
        explicit SMouseMovedEvent(const sf::Event::MouseMoved& event)
            : position(event.position)
        {
        }

        sf::Vector2i position;
    };

    struct SMouseWheelScrolledEvent : TEvent<SMouseWheelScrolledEvent>
    {
        explicit SMouseWheelScrolledEvent(const sf::Event::MouseWheelScrolled& event)
            : wheel(event.wheel)
            , delta(event.delta)
            , position(event.position)
        {
        }

        sf::Mouse::Wheel wheel;
        float delta;
        sf::Vector2i position;
    };
}
