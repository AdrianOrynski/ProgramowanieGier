#include "engine/EventHandle.hpp"

#include <utility>

#include "engine/EventDispatcher.hpp"

namespace Engine
{
    CEventHandle::CEventHandle(std::weak_ptr<CEventDispatcher> dispatcher, listener_id_t listenerId)
        : m_dispatcher(std::move(dispatcher))
        , m_listenerId(listenerId)
    {
    }

    CEventHandle::~CEventHandle()
    {
        Reset();
    }

    CEventHandle::CEventHandle(CEventHandle&& other) noexcept
        : NonCopyable(std::move(other))
        , m_dispatcher(std::move(other.m_dispatcher))
        , m_listenerId(std::exchange(other.m_listenerId, INVALID_LISTENER_ID))
    {
    }

    CEventHandle& CEventHandle::operator=(CEventHandle&& other) noexcept
    {
        if (this != &other)
        {
            // Obecna subskrypcja zostaje zakończona, zanim przejmiemy cudzą
            Reset();

            m_dispatcher = std::move(other.m_dispatcher);
            m_listenerId = std::exchange(other.m_listenerId, INVALID_LISTENER_ID);
        }
        return *this;
    }

    void CEventHandle::Reset()
    {
        if (m_listenerId == INVALID_LISTENER_ID)
            return;

        // lock() zwraca nullptr, jeżeli dispatcher już nie istnieje
        if (const std::shared_ptr<CEventDispatcher> dispatcher = m_dispatcher.lock())
            dispatcher->Unsubscribe(m_listenerId);

        m_dispatcher.reset();
        m_listenerId = INVALID_LISTENER_ID;
    }

    bool CEventHandle::IsValid() const
    {
        if (m_listenerId == INVALID_LISTENER_ID)
            return false;

        const std::shared_ptr<CEventDispatcher> dispatcher = m_dispatcher.lock();
        return dispatcher && dispatcher->IsSubscribed(m_listenerId);
    }
}
