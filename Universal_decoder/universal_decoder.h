#ifndef UNIVERSAL_DECODER_H
#define UNIVERSAL_DECODER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SEROSA_SIGNATURE "SEROSA"

// Коды алгоритмов
#define ALGORITHM_NO_COMPRESSION 0
#define ALGORITHM_HUFFMAN 1

#pragma pack(push, 1)
// Базовый заголовок
typedef struct {
    unsigned char signature[6];
    uint16_t version;
    uint16_t algorithm;
    uint64_t original_size;
} BaseHeader;

// Заголовок для алгоритма Хаффмана
typedef struct {
    BaseHeader base;
    uint64_t compressed_size_bits;
    uint16_t tree_leaf_count;
    unsigned char reserved[8];
} HuffmanHeader;
#pragma pack(pop)

// Универсальные функции
int universal_decode(const char* input_path, const char* output_path);
int detect_format_and_decode(FILE* input, FILE* output);
int read_compatible_header(FILE* input, BaseHeader* base_header, int* format_type);
void print_supported_formats();

// Декодеры
int decode_no_compression(FILE* input, FILE* output, BaseHeader* header, int format_type);
int decode_huffman(FILE* input, FILE* output, BaseHeader* base_header, int format_type);

#endif