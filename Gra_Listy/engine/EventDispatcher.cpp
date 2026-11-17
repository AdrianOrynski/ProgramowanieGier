#include "engine/EventDispatcher.hpp"

#include <algorithm>

namespace Engine
{
    CEventDispatcher::~CEventDispatcher()
    {
        // Ktoś może trzymać migawkę słuchaczy (shared_ptr) - oznaczamy ich jako wypisanych
        Clear();
    }

    listener_id_t CEventDispatcher::SubscribeErased(event_id_t eventId, callback_t callback)
    {
        const listener_id_t listenerId = m_nextListenerId++;

        auto listener = std::make_shared<SListener>();
        listener->id = listenerId;
        listener->callback = std::move(callback);

        m_listeners[eventId].push_back(std::move(listener));
        return listenerId;
    }

    void CEventDispatcher::Dispatch(const IEvent& event)
    {
        const auto it = m_listeners.find(event.GetId());
        if (it == m_listeners.end())
            return;

        // Migawka: callback może zasubskrybować / wypisać kogoś / wywołać Clear(),
        // co zmieniłoby wektor w trakcie iteracji. Kopiujemy tylko shared_ptr-y.
        // Słuchacze dodani w trakcie rozsyłania dostaną dopiero następne wydarzenie.
        const std::vector<listener_ptr_t> snapshot = it->second;

        for (const listener_ptr_t& listener : snapshot)
        {
            // Pomijamy słuchaczy wypisanych w trakcie tego rozsyłania
            if (listener->id != INVALID_LISTENER_ID)
                listener->callback(event);
        }
    }

    void CEventDispatcher::Frame(const SFrameTime& frameTime)
    {
        // Przenosimy kolejkę na bok: callbacki rozsyłanych wydarzeń mogą kolejkować nowe,
        // a te powinny zacząć odliczanie dopiero od następnej klatki
        std::vector<SQueuedEvent> pending = std::move(m_queue);
        m_queue.clear();

        std::vector<std::unique_ptr<IEvent>> ready;

        for (SQueuedEvent& queued : pending)
        {
            bool isReady = false;

            if (queued.useFrameDelay)
            {
                // delayFrames = 1 (lub 0) -> rozsyłamy w pierwszym Frame() po zakolejkowaniu
                if (queued.remainingFrames <= 1)
                    isReady = true;
                else
                    --queued.remainingFrames;
            }
            else
            {
                queued.remainingTime -= frameTime.scaledDeltaTime;
                isReady = queued.remainingTime <= 0.f;
            }

            if (isReady)
                ready.push_back(std::move(queued.event));
            else
                m_queue.push_back(std::move(queued));
        }

        // Rozsyłamy w kolejności zakolejkowania
        for (const std::unique_ptr<IEvent>& event : ready)
            Dispatch(*event);
    }

    void CEventDispatcher::Unsubscribe(listener_id_t listenerId)
    {
        if (listenerId == INVALID_LISTENER_ID)
            return;

        for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it)
        {
            std::vector<listener_ptr_t>& listeners = it->second;

            const auto found = std::find_if(listeners.begin(), listeners.end(),
                [listenerId](const listener_ptr_t& listener) { return listener->id == listenerId; });

            if (found != listeners.end())
            {
                (*found)->id = INVALID_LISTENER_ID;  // na wypadek trwającego rozsyłania (migawka)
                listeners.erase(found);

                if (listeners.empty())
                    m_listeners.erase(it);
                return;
            }
        }
    }

    void CEventDispatcher::Clear()
    {
        for (auto& [eventId, listeners] : m_listeners)
            for (const listener_ptr_t& listener : listeners)
                listener->id = INVALID_LISTENER_ID;

        m_listeners.clear();
        m_queue.clear();
    }

    bool CEventDispatcher::IsSubscribed(listener_id_t listenerId) const
    {
        if (listenerId == INVALID_LISTENER_ID)
            return false;

        for (const auto& [eventId, listeners] : m_listeners)
            for (const listener_ptr_t& listener : listeners)
                if (listener->id == listenerId)
                    return true;

        return false;
    }

    std::size_t CEventDispatcher::GetListenerCount() const
    {
        std::size_t count = 0;
        for (const auto& [eventId, listeners] : m_listeners)
            count += listeners.size();
        return count;
    }
}
