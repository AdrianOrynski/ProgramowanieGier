#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <typeinfo>

namespace Engine
{
    // (a) Typy pomocnicze systemu wydarzeń

    // Identyfikator TYPU wydarzenia (wszystkie SKeyPressedEvent mają ten sam)
    using event_id_t = std::size_t;
    constexpr event_id_t INVALID_EVENT_ID = std::numeric_limits<event_id_t>::max();

    // Identyfikator POJEDYNCZEJ subskrypcji w dispatcherze
    using listener_id_t = std::uint64_t;
    constexpr listener_id_t INVALID_LISTENER_ID = std::numeric_limits<listener_id_t>::max();

    // (b) Wspólny interfejs wszystkich wydarzeń - pozwala traktować je polimorficznie (const IEvent&)
    class IEvent
    {
    public:
        virtual ~IEvent() = default;

        virtual event_id_t GetId() const = 0;
        virtual std::string_view GetName() const = 0;
    };

    // CRTP: konkretne wydarzenie dziedziczy po TEvent<samo siebie> i automatycznie
    // dostaje identyfikator oraz nazwę typu:
    //
    //     struct SPlayerDiedEvent : TEvent<SPlayerDiedEvent>
    //     {
    //         explicit SPlayerDiedEvent(int id) : playerId(id) {}
    //         int playerId;
    //     };
    //
    // Uwaga (slajd 31): hash_code() nie gwarantuje unikalności między typami i nie jest
    // stabilny między uruchomieniami - nie nadaje się do serializacji (np. zapisu gry).
    template <typename T>
    class TEvent : public IEvent
    {
    public:
        // Wersje statyczne - dispatcher zna typ w czasie kompilacji (Subscribe<T>), bez obiektu
        static event_id_t GetStaticEventId() { return typeid(T).hash_code(); }
        static std::string_view GetStaticEventName() { return typeid(T).name(); }

        event_id_t GetId() const override { return GetStaticEventId(); }
        std::string_view GetName() const override { return GetStaticEventName(); }
    };

    // Typ T jest poprawnym wydarzeniem, jeżeli dziedziczy po TEvent<T>.
    // Chroni przed pomyłkami typu "struct SFoo : TEvent<SBar>", przy których
    // static_cast w dispatcherze dałby niezdefiniowane zachowanie.
    template <typename T>
    concept EventType = std::derived_from<T, TEvent<T>>;
}
