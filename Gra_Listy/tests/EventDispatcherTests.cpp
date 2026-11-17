// Testy jednostkowe systemu wydarzeń (lista 2, zadanie 7)

#include <catch_amalgamated.hpp>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/EventDispatcher.hpp"
#include "engine/EventHandle.hpp"
#include "engine/FrameTime.hpp"

using namespace Engine;

namespace
{
    // (a) Pomocnicze typy wydarzeń

    struct SEmptyEvent : TEvent<SEmptyEvent>
    {
        SEmptyEvent() = default;
    };

    struct SIntEvent : TEvent<SIntEvent>
    {
        explicit SIntEvent(int value)
            : value(value)
        {
        }

        int value;
    };

    struct SFloatEvent : TEvent<SFloatEvent>
    {
        explicit SFloatEvent(float value)
            : value(value)
        {
        }

        float value;
    };

    struct SStringEvent : TEvent<SStringEvent>
    {
        explicit SStringEvent(std::string value)
            : value(std::move(value))
        {
        }

        std::string value;
    };

    // (b) Fixture: czas klatki + dispatcher (w shared_ptr - wymagane przez CEventHandle)
    struct SEventDispatcherFixture
    {
        static constexpr float DEFAULT_DT = 0.1f;

        SFrameTime frameTime;
        std::shared_ptr<CEventDispatcher> dispatcher = std::make_shared<CEventDispatcher>();

        // (c) Jedna klatka: przesuwa czas i wywołuje CEventDispatcher::Frame().
        //
        // (e) Dlaczego dt + epsilon?
        // Opóźnienia czasowe są odliczane przez odejmowanie scaledDeltaTime od pozostałego czasu.
        // Wartości takich jak 0.1 nie da się dokładnie zapisać w float (to ułamek nieskończony
        // w systemie dwójkowym), a każde odejmowanie dodatkowo zaokrągla wynik. Po 10 klatkach
        // po 0.1 s z opóźnienia 1.0 s zostaje więc np. +3e-8 zamiast 0 - warunek "<= 0" nie jest
        // spełniony i wydarzenie przychodzi klatkę za późno, choć matematycznie czas już minął.
        // Dodanie epsilon (najmniejszej różnicy między 1.0f a następnym floatem, ~1.19e-7) do
        // każdego kroku sprawia, że suma kroków minimalnie PRZEKRACZA zadany czas, zamiast
        // minimalnie go nie osiągać. Epsilon jest o rzędy wielkości mniejszy od dt, więc nie
        // zmienia wyniku dla klatek wcześniejszych (po 9 klatkach nadal brakuje ~0.1 s).
        // Testy sprawdzają w ten sposób logikę dispatchera, a nie szum zaokrągleń.
        void Tick(float dt = DEFAULT_DT)
        {
            frameTime.Update(dt + std::numeric_limits<float>::epsilon());
            dispatcher->Frame(frameTime);
        }

        // (c) Przeciążenie: wiele klatek naraz
        void Tick(int frames, float dt = DEFAULT_DT)
        {
            for (int i = 0; i < frames; ++i)
                Tick(dt);
        }
    };
}

// --- Identyfikatory typów ---

TEST_CASE("Identyfikatory typów wydarzeń", "[events][id]")
{
    SECTION("ten sam typ ma zawsze ten sam identyfikator")
    {
        REQUIRE(SIntEvent::GetStaticEventId() == SIntEvent::GetStaticEventId());
        REQUIRE(SIntEvent(1).GetId() == SIntEvent(2).GetId());
        REQUIRE(SIntEvent(1).GetId() == SIntEvent::GetStaticEventId());
    }

    SECTION("różne typy mają różne identyfikatory")
    {
        const std::vector<event_id_t> ids = {
            SEmptyEvent::GetStaticEventId(),
            SIntEvent::GetStaticEventId(),
            SFloatEvent::GetStaticEventId(),
            SStringEvent::GetStaticEventId(),
        };

        for (std::size_t i = 0; i < ids.size(); ++i)
        {
            CHECK(ids[i] != INVALID_EVENT_ID);
            for (std::size_t j = i + 1; j < ids.size(); ++j)
                CHECK(ids[i] != ids[j]);
        }
    }

    SECTION("identyfikator i nazwa działają polimorficznie przez IEvent")
    {
        const SStringEvent event("tekst");
        const IEvent& base = event;

        REQUIRE(base.GetId() == SStringEvent::GetStaticEventId());
        REQUIRE(base.GetName() == SStringEvent::GetStaticEventName());
        REQUIRE(base.GetName().find("SStringEvent") != std::string_view::npos);
    }
}

// --- Subscribe / Submit ---

TEST_CASE_METHOD(SEventDispatcherFixture, "Subscribe i Submit przekazują dane wydarzenia", "[events][submit]")
{
    int receivedInt = 0;
    float receivedFloat = 0.f;
    std::string receivedString;
    int emptyCount = 0;

    auto intHandle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { receivedInt = e.value; });
    auto floatHandle = dispatcher->Subscribe<SFloatEvent>([&](const SFloatEvent& e) { receivedFloat = e.value; });
    auto stringHandle = dispatcher->Subscribe<SStringEvent>([&](const SStringEvent& e) { receivedString = e.value; });
    auto emptyHandle = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++emptyCount; });

    dispatcher->Submit<SIntEvent>(42);
    dispatcher->Submit<SFloatEvent>(3.5f);
    dispatcher->Submit<SStringEvent>("hello");      // const char* -> std::string przez konstruktor
    dispatcher->Submit<SEmptyEvent>();

    // Submit rozsyła natychmiast - bez Frame()
    REQUIRE(receivedInt == 42);
    REQUIRE(receivedFloat == 3.5f);
    REQUIRE(receivedString == "hello");
    REQUIRE(emptyCount == 1);
}

TEST_CASE_METHOD(SEventDispatcherFixture, "Submit bez słuchaczy nic nie robi", "[events][submit]")
{
    REQUIRE_NOTHROW(dispatcher->Submit<SIntEvent>(1));
    REQUIRE(dispatcher->GetListenerCount() == 0);
}

// --- Wielu odbiorców i różne typy ---

TEST_CASE_METHOD(SEventDispatcherFixture, "Wielu odbiorców i różne typy wydarzeń", "[events][listeners]")
{
    std::vector<int> calls;

    auto first = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { calls.push_back(e.value * 1); });
    auto second = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { calls.push_back(e.value * 10); });
    auto third = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { calls.push_back(e.value * 100); });

    int floatCalls = 0;
    auto floatHandle = dispatcher->Subscribe<SFloatEvent>([&](const SFloatEvent&) { ++floatCalls; });

    REQUIRE(dispatcher->GetListenerCount() == 4);

    SECTION("wszyscy odbiorcy danego typu dostają wydarzenie, w kolejności subskrypcji")
    {
        dispatcher->Submit<SIntEvent>(2);

        REQUIRE(calls == std::vector<int>{ 2, 20, 200 });
        REQUIRE(floatCalls == 0);
    }

    SECTION("wydarzenie innego typu nie trafia do słuchaczy SIntEvent")
    {
        dispatcher->Submit<SFloatEvent>(1.f);

        REQUIRE(calls.empty());
        REQUIRE(floatCalls == 1);
    }

    SECTION("wypisanie jednego odbiorcy nie wpływa na pozostałych")
    {
        dispatcher->Unsubscribe(second.GetListenerId());
        dispatcher->Submit<SIntEvent>(3);

        REQUIRE(calls == std::vector<int>{ 3, 300 });
        REQUIRE(dispatcher->GetListenerCount() == 3);
    }
}

// --- CEventHandle, Reset() i move semantics ---

TEST_CASE_METHOD(SEventDispatcherFixture, "CEventHandle zarządza czasem życia subskrypcji", "[events][handle]")
{
    int calls = 0;
    auto callback = [&](const SEmptyEvent&) { ++calls; };

    SECTION("domyślny uchwyt jest nieważny")
    {
        CEventHandle handle;
        REQUIRE_FALSE(handle.IsValid());
        REQUIRE(handle.GetListenerId() == INVALID_LISTENER_ID);
        REQUIRE_NOTHROW(handle.Reset());
    }

    SECTION("zniszczenie uchwytu wypisuje słuchacza (RAII)")
    {
        {
            CEventHandle handle = dispatcher->Subscribe<SEmptyEvent>(callback);
            REQUIRE(handle.IsValid());

            dispatcher->Submit<SEmptyEvent>();
            REQUIRE(calls == 1);
        }

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 1);
        REQUIRE(dispatcher->GetListenerCount() == 0);
    }

    SECTION("Reset() wypisuje słuchacza i unieważnia uchwyt")
    {
        CEventHandle handle = dispatcher->Subscribe<SEmptyEvent>(callback);
        handle.Reset();

        REQUIRE_FALSE(handle.IsValid());
        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 0);

        REQUIRE_NOTHROW(handle.Reset());    // ponowny Reset jest bezpieczny
    }

    SECTION("konstruktor przenoszący przekazuje własność subskrypcji")
    {
        CEventHandle original = dispatcher->Subscribe<SEmptyEvent>(callback);
        const listener_id_t id = original.GetListenerId();

        CEventHandle moved(std::move(original));

        REQUIRE_FALSE(original.IsValid());  // NOLINT(bugprone-use-after-move) - celowo
        REQUIRE(moved.IsValid());
        REQUIRE(moved.GetListenerId() == id);

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 1);                // dokładnie jedna subskrypcja, nie dwie
    }

    SECTION("operator przenoszący kończy poprzednią subskrypcję celu")
    {
        int otherCalls = 0;
        CEventHandle target = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++otherCalls; });
        CEventHandle source = dispatcher->Subscribe<SEmptyEvent>(callback);

        target = std::move(source);

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 1);
        REQUIRE(otherCalls == 0);
        REQUIRE(dispatcher->GetListenerCount() == 1);
    }

    SECTION("uchwyty przeniesione do kontenera nadal działają")
    {
        std::vector<CEventHandle> handles;
        for (int i = 0; i < 10; ++i)
            handles.push_back(dispatcher->Subscribe<SEmptyEvent>(callback));   // realokacje wektora

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 10);

        handles.clear();
        REQUIRE(dispatcher->GetListenerCount() == 0);
    }

    SECTION("uchwyt przeżywający dispatcher jest bezpieczny (weak_ptr)")
    {
        CEventHandle handle = dispatcher->Subscribe<SEmptyEvent>(callback);
        dispatcher.reset();

        REQUIRE_FALSE(handle.IsValid());
        REQUIRE_NOTHROW(handle.Reset());
        // destruktor uchwytu na końcu sekcji też nie może się odwołać do zniszczonego dispatchera
    }
}

// --- Wydarzenia opóźnione o klatki ---

TEST_CASE_METHOD(SEventDispatcherFixture, "QueueWithFrameDelay rozsyła po zadanej liczbie klatek", "[events][queue]")
{
    std::vector<int> received;
    auto handle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { received.push_back(e.value); });

    SECTION("opóźnienie 3 klatek")
    {
        dispatcher->QueueWithFrameDelay<SIntEvent>(3, 7);
        REQUIRE(received.empty());          // samo zakolejkowanie niczego nie rozsyła
        REQUIRE(dispatcher->GetQueuedEventCount() == 1);

        Tick(2);
        REQUIRE(received.empty());

        Tick();
        REQUIRE(received == std::vector<int>{ 7 });
        REQUIRE(dispatcher->GetQueuedEventCount() == 0);

        Tick(10);
        REQUIRE(received.size() == 1);      // rozesłane tylko raz
    }

    SECTION("opóźnienie 0 i 1 oznacza najbliższą klatkę")
    {
        dispatcher->QueueWithFrameDelay<SIntEvent>(0, 0);
        dispatcher->QueueWithFrameDelay<SIntEvent>(1, 1);

        Tick();
        REQUIRE(received == std::vector<int>{ 0, 1 });
    }

    SECTION("wydarzenia gotowe w tej samej klatce przychodzą w kolejności zakolejkowania")
    {
        dispatcher->QueueWithFrameDelay<SIntEvent>(2, 1);
        dispatcher->QueueWithFrameDelay<SIntEvent>(1, 2);
        dispatcher->QueueWithFrameDelay<SIntEvent>(2, 3);

        Tick();
        REQUIRE(received == std::vector<int>{ 2 });

        Tick();
        REQUIRE(received == std::vector<int>{ 2, 1, 3 });
    }

    SECTION("opóźnienie klatkowe nie zależy od TimeScale")
    {
        frameTime.timeScale = 0.f;
        dispatcher->QueueWithFrameDelay<SIntEvent>(2, 5);

        Tick(2);
        REQUIRE(received == std::vector<int>{ 5 });
    }
}

// --- Wydarzenia opóźnione o czas ---

TEST_CASE_METHOD(SEventDispatcherFixture, "QueueWithTimeDelay rozsyła po zadanym czasie", "[events][queue]")
{
    std::vector<std::string> received;
    auto handle = dispatcher->Subscribe<SStringEvent>([&](const SStringEvent& e) { received.push_back(e.value); });

    SECTION("1 s przy klatkach po 0.1 s = 10 klatek")
    {
        dispatcher->QueueWithTimeDelay<SStringEvent>(1.f, "po sekundzie");

        Tick(9);
        REQUIRE(received.empty());

        Tick();
        REQUIRE(received == std::vector<std::string>{ "po sekundzie" });
    }

    SECTION("1 s przy 60 FPS = 60 klatek")
    {
        dispatcher->QueueWithTimeDelay<SStringEvent>(1.f, "60 fps");

        Tick(59, 1.f / 60.f);
        REQUIRE(received.empty());

        Tick(1.f / 60.f);
        REQUIRE(received.size() == 1);
    }

    SECTION("różne opóźnienia rozsyłane we właściwych klatkach")
    {
        dispatcher->QueueWithTimeDelay<SStringEvent>(0.5f, "pol");
        dispatcher->QueueWithTimeDelay<SStringEvent>(0.2f, "dwie dziesiate");

        Tick(2);
        REQUIRE(received == std::vector<std::string>{ "dwie dziesiate" });

        Tick(3);
        REQUIRE(received == std::vector<std::string>{ "dwie dziesiate", "pol" });
    }

    SECTION("opóźnienie 0 oznacza najbliższą klatkę")
    {
        dispatcher->QueueWithTimeDelay<SStringEvent>(0.f, "teraz");
        REQUIRE(received.empty());

        Tick();
        REQUIRE(received.size() == 1);
    }
}

// --- Wpływ TimeScale ---

TEST_CASE_METHOD(SEventDispatcherFixture, "TimeScale wpływa na opóźnienia czasowe", "[events][queue][timescale]")
{
    int calls = 0;
    auto handle = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++calls; });

    dispatcher->QueueWithTimeDelay<SEmptyEvent>(1.f);

    SECTION("TimeScale = 2 skraca oczekiwanie o połowę")
    {
        frameTime.timeScale = 2.f;

        Tick(4);
        REQUIRE(calls == 0);
        Tick();
        REQUIRE(calls == 1);
    }

    SECTION("TimeScale = 0.5 wydłuża oczekiwanie dwukrotnie")
    {
        frameTime.timeScale = 0.5f;

        Tick(19);
        REQUIRE(calls == 0);
        Tick();
        REQUIRE(calls == 1);
    }

    SECTION("TimeScale = 0 (pauza) wstrzymuje odliczanie")
    {
        frameTime.timeScale = 0.f;

        Tick(100);
        REQUIRE(calls == 0);
        REQUIRE(dispatcher->GetQueuedEventCount() == 1);

        // Po wznowieniu odliczanie startuje od pełnej sekundy - pauza niczego nie "zjadła"
        frameTime.timeScale = 1.f;
        Tick(9);
        REQUIRE(calls == 0);
        Tick();
        REQUIRE(calls == 1);
    }

    SECTION("zmiana TimeScale w trakcie odliczania")
    {
        Tick(5);                            // 0.5 s czasu skalowanego
        frameTime.timeScale = 0.5f;

        Tick(9);                            // + 0.45 s
        REQUIRE(calls == 0);
        Tick();                             // + 0.05 s = 1.0 s
        REQUIRE(calls == 1);
    }
}

// --- Clear() i wielokrotne wysyłanie ---

TEST_CASE_METHOD(SEventDispatcherFixture, "Clear() usuwa słuchaczy i kolejkę", "[events][clear]")
{
    int calls = 0;
    auto intHandle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent&) { ++calls; });
    auto emptyHandle = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++calls; });

    dispatcher->QueueWithFrameDelay<SIntEvent>(1, 1);
    dispatcher->QueueWithTimeDelay<SEmptyEvent>(0.1f);

    dispatcher->Clear();

    REQUIRE(dispatcher->GetListenerCount() == 0);
    REQUIRE(dispatcher->GetQueuedEventCount() == 0);
    REQUIRE_FALSE(intHandle.IsValid());

    dispatcher->Submit<SIntEvent>(1);
    Tick(5);
    REQUIRE(calls == 0);

    // Uchwyty po Clear() można bezpiecznie zresetować / zniszczyć
    REQUIRE_NOTHROW(intHandle.Reset());

    SECTION("po Clear() można normalnie subskrybować od nowa")
    {
        auto newHandle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent&) { ++calls; });
        dispatcher->Submit<SIntEvent>(1);
        REQUIRE(calls == 1);
    }
}

TEST_CASE_METHOD(SEventDispatcherFixture, "Wielokrotne wysyłanie wydarzeń", "[events][submit]")
{
    int sum = 0;
    int calls = 0;
    auto handle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent& e) { sum += e.value; ++calls; });

    for (int i = 1; i <= 100; ++i)
        dispatcher->Submit<SIntEvent>(i);

    REQUIRE(calls == 100);
    REQUIRE(sum == 5050);

    SECTION("mieszanie Submit i kolejki")
    {
        dispatcher->QueueWithFrameDelay<SIntEvent>(1, 1000);
        dispatcher->Submit<SIntEvent>(1);
        REQUIRE(sum == 5051);

        Tick();
        REQUIRE(sum == 6051);
        REQUIRE(calls == 102);
    }
}

// --- Modyfikacja dispatchera z wnętrza callbacka ---

TEST_CASE_METHOD(SEventDispatcherFixture, "Callback może bezpiecznie modyfikować dispatcher", "[events][reentrancy]")
{
    SECTION("wypisanie innego słuchacza w trakcie rozsyłania")
    {
        int secondCalls = 0;
        CEventHandle second;

        auto first = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { second.Reset(); });
        second = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++secondCalls; });

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(secondCalls == 0);          // wypisany przed swoją kolejką - nie dostaje wydarzenia
    }

    SECTION("wypisanie samego siebie w trakcie rozsyłania")
    {
        int calls = 0;
        CEventHandle self;
        self = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++calls; self.Reset(); });

        dispatcher->Submit<SEmptyEvent>();
        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(calls == 1);
    }

    SECTION("subskrypcja w trakcie rozsyłania obowiązuje od następnego wydarzenia")
    {
        int lateCalls = 0;
        std::vector<CEventHandle> lateHandles;

        auto first = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&)
        {
            lateHandles.push_back(dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++lateCalls; }));
        });

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(lateCalls == 0);

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(lateCalls == 1);            // pierwszy "późny" słuchacz (drugi dopiero się dopisał)
    }

    SECTION("wydarzenie zakolejkowane z callbacka w Frame() czeka do następnej klatki")
    {
        int intCalls = 0;
        auto emptyHandle = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&)
        {
            dispatcher->QueueWithFrameDelay<SIntEvent>(1, 0);
        });
        auto intHandle = dispatcher->Subscribe<SIntEvent>([&](const SIntEvent&) { ++intCalls; });

        dispatcher->QueueWithFrameDelay<SEmptyEvent>(1);

        Tick();
        REQUIRE(intCalls == 0);
        Tick();
        REQUIRE(intCalls == 1);
    }

    SECTION("Clear() w trakcie rozsyłania zatrzymuje pozostałych słuchaczy")
    {
        int secondCalls = 0;
        auto first = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { dispatcher->Clear(); });
        auto second = dispatcher->Subscribe<SEmptyEvent>([&](const SEmptyEvent&) { ++secondCalls; });

        dispatcher->Submit<SEmptyEvent>();
        REQUIRE(secondCalls == 0);
        REQUIRE(dispatcher->GetListenerCount() == 0);
    }
}
