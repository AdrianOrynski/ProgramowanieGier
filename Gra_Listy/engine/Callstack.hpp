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

    // Ramka w postaci "Funkcja (plik:linia) [0xadres]"
    std::string FormatStackFrame(const SStackFrame& frame);
}
