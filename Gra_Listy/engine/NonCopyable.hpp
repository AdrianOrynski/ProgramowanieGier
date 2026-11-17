#pragma once

namespace Engine
{
    // Klasa bazowa usuwająca kopiowanie w klasach dziedziczących (zasada DRY):
    //     class CFoo : private NonCopyable { ... };
    //
    // Konstruktor i destruktor są protected - NonCopyable nie jest typem samym w sobie,
    // a brak wirtualnego destruktora nie pozwala usuwać obiektów przez wskaźnik na bazę.
    //
    // Przenoszenie jest jawnie domyślne: zadeklarowanie (= delete) operacji kopiujących
    // blokuje niejawne generowanie operacji przenoszących (rule of five), więc bez tego
    // klasa dziedzicząca nie mogłaby być przenoszalna nawet po zdefiniowaniu własnego move.
    class NonCopyable
    {
    public:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;

    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(NonCopyable&&) noexcept = default;
        NonCopyable& operator=(NonCopyable&&) noexcept = default;
    };
}

// Wariant makra do użycia w ciele klasy, gdy nie chcemy dziedziczyć:
//     class CFoo { NON_COPYABLE(CFoo); public: ... };
// Usuwa tylko kopiowanie - przenoszenie klasa może zdefiniować sama.
#define NON_COPYABLE(ClassName)                       \
public:                                               \
    ClassName(const ClassName&) = delete;             \
    ClassName& operator=(const ClassName&) = delete;  \
private:
