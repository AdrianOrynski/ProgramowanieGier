#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/Event.hpp"
#include "engine/EventHandle.hpp"
#include "engine/FrameTime.hpp"
#include "engine/NonCopyable.hpp"

namespace Engine
{
    // (c) Centralny element systemu wydarzeń (wzorzec obserwatora): rejestruje słuchaczy
    // pogrupowanych po identyfikatorze typu wydarzenia i rozsyła do nich wydarzenia.
    //
    // Dispatcher musi być zarządzany przez std::shared_ptr (std::make_shared<CEventDispatcher>()),
    // bo uchwyty CEventHandle trzymają do niego weak_ptr (enable_shared_from_this).
    //
    // Bezpieczne jest wywoływanie Subscribe / Unsubscribe / Submit / Queue* / Clear z wnętrza
    // callbacka w trakcie rozsyłania. Klasa nie jest wielowątkowa.
    class CEventDispatcher : public std::enable_shared_from_this<CEventDispatcher>, private NonCopyable
    {
    public:
        // Callback po type erasure - wspólna sygnatura dla wszystkich typów wydarzeń
        using callback_t = std::function<void(const IEvent&)>;

        CEventDispatcher() = default;
        ~CEventDispatcher();

        // (d) Rejestruje callback void(const TEventType&). Zwraca uchwyt RAII - jego zniszczenie
        // wypisuje słuchacza, dlatego [[nodiscard]]: zignorowany uchwyt od razu kończy subskrypcję.
        template <EventType TEventType>
        [[nodiscard]] CEventHandle Subscribe(std::function<void(const TEventType&)> callback);

        // (e) Konstruuje wydarzenie z argumentów (perfect forwarding) i natychmiast je rozsyła
        template <EventType TEventType, typename... Args>
        void Submit(Args&&... args);

        // (f) Kolejkuje wydarzenie - zostanie rozesłane w Frame(), gdy upłynie delaySeconds
        // czasu SKALOWANEGO (SFrameTime::scaledDeltaTime - pauza wstrzymuje odliczanie)
        template <EventType TEventType, typename... Args>
        void QueueWithTimeDelay(float delaySeconds, Args&&... args);

        // (f) Kolejkuje wydarzenie - zostanie rozesłane w Frame() po upływie delayFrames klatek
        // (1 = w najbliższym Frame(); 0 działa tak samo - szybciej się nie da)
        template <EventType TEventType, typename... Args>
        void QueueWithFrameDelay(std::uint64_t delayFrames, Args&&... args);

        // (f) Wywoływane raz na klatkę: odlicza opóźnienia i rozsyła wydarzenia, które już "dojrzały"
        void Frame(const SFrameTime& frameTime);

        // (g) Wypisuje słuchacza o danym identyfikatorze (nieznany identyfikator jest ignorowany)
        void Unsubscribe(listener_id_t listenerId);

        // (g) Usuwa wszystkich słuchaczy i wszystkie zakolejkowane wydarzenia
        void Clear();

        bool IsSubscribed(listener_id_t listenerId) const;
        std::size_t GetListenerCount() const;
        std::size_t GetQueuedEventCount() const { return m_queue.size(); }

    private:
        struct SListener
        {
            listener_id_t id = INVALID_LISTENER_ID;
            callback_t callback;
        };

        // shared_ptr, żeby rozsyłanie mogło pracować na kopii listy (migawce), a callback
        // mógł w tym czasie bezpiecznie modyfikować oryginał. Wypisany słuchacz dostaje
        // id = INVALID_LISTENER_ID, więc migawka go pominie.
        using listener_ptr_t = std::shared_ptr<SListener>;

        struct SQueuedEvent
        {
            std::unique_ptr<IEvent> event;
            bool useFrameDelay = false;
            float remainingTime = 0.f;
            std::uint64_t remainingFrames = 0;
        };

        listener_id_t SubscribeErased(event_id_t eventId, callback_t callback);
        void Dispatch(const IEvent& event);

        std::unordered_map<event_id_t, std::vector<listener_ptr_t>> m_listeners;
        std::vector<SQueuedEvent> m_queue;
        listener_id_t m_nextListenerId = 0;
    };

    // --- Implementacja szablonów ---

    template <EventType TEventType>
    CEventHandle CEventDispatcher::Subscribe(std::function<void(const TEventType&)> callback)
    {
        // Type erasure: lambda o wspólnej sygnaturze void(const IEvent&) przechwytuje
        // konkretny callback. static_cast jest bezpieczny - pod tym event_id są tylko TEventType.
        callback_t typeErasedCallback = [callback = std::move(callback)](const IEvent& event)
        {
            callback(static_cast<const TEventType&>(event));
        };

        const listener_id_t listenerId = SubscribeErased(TEventType::GetStaticEventId(), std::move(typeErasedCallback));

        // weak_from_this() jest pusty, jeżeli dispatcher nie należy do shared_ptr -
        // wtedy uchwyt po prostu nie wypisze słuchacza (zrobi to Clear() lub destruktor)
        return CEventHandle(weak_from_this(), listenerId);
    }

    template <EventType TEventType, typename... Args>
    void CEventDispatcher::Submit(Args&&... args)
    {
        const TEventType event(std::forward<Args>(args)...);
        Dispatch(event);
    }

    template <EventType TEventType, typename... Args>
    void CEventDispatcher::QueueWithTimeDelay(float delaySeconds, Args&&... args)
    {
        SQueuedEvent queued;
        queued.event = std::make_unique<TEventType>(std::forward<Args>(args)...);
        queued.useFrameDelay = false;
        queued.remainingTime = delaySeconds;
        m_queue.push_back(std::move(queued));
    }

    template <EventType TEventType, typename... Args>
    void CEventDispatcher::QueueWithFrameDelay(std::uint64_t delayFrames, Args&&... args)
    {
        SQueuedEvent queued;
        queued.event = std::make_unique<TEventType>(std::forward<Args>(args)...);
        queued.useFrameDelay = true;
        queued.remainingFrames = delayFrames;
        m_queue.push_back(std::move(queued));
    }
}
