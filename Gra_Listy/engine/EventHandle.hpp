#pragma once

#include <memory>

#include "engine/Event.hpp"
#include "engine/NonCopyable.hpp"

namespace Engine
{
    class CEventDispatcher;

    // (h) Uchwyt RAII reprezentujący jedną subskrypcję, zwracany przez CEventDispatcher::Subscribe.
    // W destruktorze automatycznie wypisuje słuchacza.
    //
    // Niekopiowalny (kto miałby wykonać Unsubscribe?), ale przenoszalny - po przeniesieniu
    // tylko nowy obiekt jest właścicielem subskrypcji.
    //
    // Przechowuje weak_ptr: nie przedłuża życia dispatchera, a jeżeli dispatcher zostanie
    // zniszczony wcześniej, destruktor uchwytu po prostu nic nie robi (brak dangling pointera).
    class CEventHandle : private NonCopyable
    {
    public:
        CEventHandle() = default;
        CEventHandle(std::weak_ptr<CEventDispatcher> dispatcher, listener_id_t listenerId);
        ~CEventHandle();

        CEventHandle(CEventHandle&& other) noexcept;
        CEventHandle& operator=(CEventHandle&& other) noexcept;

        // Wypisuje słuchacza (jeśli dispatcher jeszcze istnieje) i unieważnia uchwyt
        void Reset();

        // Czy uchwyt nadal reprezentuje aktywną subskrypcję
        bool IsValid() const;

        listener_id_t GetListenerId() const { return m_listenerId; }

    private:
        std::weak_ptr<CEventDispatcher> m_dispatcher;
        listener_id_t m_listenerId = INVALID_LISTENER_ID;
    };
}
