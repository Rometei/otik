// common.h - общие константы и структуры
#pragma once
#include <cstdint>
#include <cstddef>

namespace Common {
    const uint32_t SIGNATURE = 0x46465548; // "HUFF" в little-endian
    const uint8_t VERSION_1 = 1;
    const uint8_t VERSION_2 = 2;
    const uint8_t VERSION_3 = 3; // Версия для Шеннона-Фано
    
    enum Algorithm : uint8_t {
        ALGO_HUFFMAN = 1,
        ALGO_HUFFMAN_CANONICAL = 2,
        ALGO_SHANNON_FANO = 3
    };
    
    const size_t ALPHABET_SIZE = 256;
}