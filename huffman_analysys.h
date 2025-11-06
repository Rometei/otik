#ifndef HUFFMAN_ANALYSIS_H
#define HUFFMAN_ANALYSIS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef struct HuffmanNode {
    unsigned char symbol;
    uint64_t frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    uint32_t code;
    uint8_t length;
} HuffmanCode;

// Основные функции анализа
void analyze_file_optimal_bits(const char* filename);
uint64_t calculate_compressed_size(uint64_t* frequencies, HuffmanCode* codes);
void normalize_frequencies(uint64_t* src_freqs, uint64_t* dst_freqs, int bits, uint64_t file_size);
HuffmanNode* build_huffman_tree(uint64_t* frequencies);
void build_huffman_codes(HuffmanNode* root, HuffmanCode* codes, uint32_t code, uint8_t depth);
void free_huffman_tree(HuffmanNode* node);

// Вспомогательные функции
uint64_t* calculate_frequencies(const char* filename, uint64_t* file_size);
int compare_nodes(const void* a, const void* b);
void print_results_table(uint64_t file_size);

#endif