#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Engine
{
    // Jedna ramka stosu wywołań po symbolizacji (adres -> funkcja, plik, linia)
    struct SStackFrame
    {
        std::uintptr_t address = 0;
        std::string function;   // pusta, jeżeli brak symboli debugowych (.pdb)
        std::string file;       // pusta, jeżeli brak informacji o liniach
        unsigned int line = 0;
    };

    // Stos wywołań w miejscu wywołania tej funkcji (ona sama jest pomijana).
    // skipFrames pozwala pominąć dodatkowe ramki, np. funkcji raportujących błąd.
    // Na Windows korzysta z DbgHelp; na innych platformach zwraca pustą listę.
    std::vector<SStackFrame> CaptureCallstack(unsigned int skipFrames = 0, unsigned int maxFrames = 62);

    // Stos wywołań odtworzony z kontekstu procesora zapisanego w chwili wyjątku
    // (Windows: wskaźnik na CONTEXT, np. EXCEPTION_POINTERS::ContextRecord).
    // W przeciwieństwie do CaptureCallstack zaczyna się od instrukcji, która spowodowała
    // wyjątek, a nie od miejsca wywołania. void*, żeby nagłówek nie zależał od Windows.h.
    std::vector<SStackFrame> CaptureCallstackFromContext(const void* platformContext, unsigned int maxFrames = 64);

    // Ramka w postaci "Funkcja (plik:linia) [0xadres]"
    std::string FormatStackFrame(const SStackFrame& frame);
}
