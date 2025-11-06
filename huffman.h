#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SEROSA_SIGNATURE "SEROSA"
#define HUFFMAN_ALGORITHM_CODE 1
#define HUFFMAN_VERSION 2

#pragma pack(push, 1)
typedef struct {
    unsigned char signature[6];
    uint16_t major_version;
    uint16_t minor_version;
    unsigned char compression_no_context;
    unsigned char compression_with_context;
    unsigned char error_protection;
    uint64_t original_size;
    uint64_t compressed_size_bits;
    uint16_t tree_leaf_count;
    unsigned char reserved[3];
} HuffmanHeader;
#pragma pack(pop)

typedef struct HuffmanNode {
    unsigned char symbol;
    int frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    uint32_t code;
    uint8_t length;
} HuffmanCode;

// Основные функции
int huffman_compress(const char* input_file, const char* output_file);
int huffman_decompress(const char* input_file, const char* output_file);

// Функции анализа для Л2.№1
void analyze_file(const char* filename);
double calculate_information_bits(int* frequencies, uint64_t file_size);

#endif