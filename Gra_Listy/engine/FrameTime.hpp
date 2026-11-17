#pragma once

#include <cstdint>

namespace Engine
{
    // Jedno źródło informacji o czasie dla całego silnika.
    // Wszystkie czasy w sekundach.
    struct SFrameTime
    {
        float deltaTime = 0.f;          // (a) czas trwania ostatniej klatki
        std::uint64_t frameIndex = 0;   // (b) indeks klatki od startu programu
        float elapsedTime = 0.f;        // (c) czas od startu programu

        float timeScale = 1.f;          // (d) mnożnik czasu (0 = pauza, 0.5 = slow-motion)

        float scaledDeltaTime = 0.f;    // (e) deltaTime * timeScale
        float scaledElapsedTime = 0.f;  // (f) suma scaledDeltaTime od startu programu

        // (g) Jedna metoda aktualizująca wszystkie pola na podstawie czasu ostatniej klatki
        void Update(float dt)
        {
            deltaTime = dt;
            ++frameIndex;

            elapsedTime += dt;

            // Czas skalowany sumujemy klatka po klatce, a nie liczymy jako elapsedTime * timeScale,
            // żeby zmiana mnożnika w trakcie gry nie "przeskakiwała" czasu wstecz lub do przodu
            scaledDeltaTime = dt * timeScale;
            scaledElapsedTime += scaledDeltaTime;
        }
    };
}
