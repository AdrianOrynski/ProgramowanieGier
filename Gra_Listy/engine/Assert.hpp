#pragma once

#include <string_view>

namespace Engine
{
    // (d) Obsługa niespełnionej asercji: loguje warunek, wiadomość, plik, linię i funkcję
    // oraz stos wywołań (e), spłukuje logi, wywołuje breakpoint, jeśli podpięty jest debugger (b),
    // i zatrzymuje program. Nie wraca.
    //
    // Breakpoint zatrzyma debugger wewnątrz tej funkcji - miejsce asercji jest
    // jedną ramkę wyżej w Call Stack (oraz w logu).
    [[noreturn]] void HandleAssertionFailure(std::string_view condition, std::string_view message,
                                             const char* file, int line, const char* function);
}

// (a) Asercje istnieją tylko w konfiguracji Debug (premake definiuje DEBUG)
#if defined(DEBUG)

    // (c) ASSERT(condition, message) - przekazuje dalej warunek jako tekst, plik, linię i funkcję.
    // do { } while (false) sprawia, że makro zachowuje się jak pojedyncza instrukcja
    // (bezpieczne w if/else bez klamer, wymaga średnika na końcu).
    #define ASSERT(condition, message)                                                          \
        do                                                                                      \
        {                                                                                       \
            if (!(condition))                                                                   \
                ::Engine::HandleAssertionFailure(#condition, (message), __FILE__, __LINE__, __func__); \
        } while (false)

#else

    // W Release warunek NIE jest wykonywany. sizeof tworzy kontekst niewartościowany:
    // kompilator sprawdza poprawność wyrażenia i nie zgłasza ostrzeżeń o nieużywanych zmiennych.
    #define ASSERT(condition, message)  \
        do                              \
        {                               \
            (void)sizeof(condition);    \
        } while (false)

#endif
