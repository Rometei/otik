#include "huffman.h"
#include <limits.h>

// Создание нового узла
HuffmanNode* create_node(unsigned char symbol, int frequency) {
    HuffmanNode* node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = node->right = NULL;
    return node;
}

// Освобождение дерева
void free_huffman_tree(HuffmanNode* node) {
    if (node == NULL) return;
    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}

// Подсчет частот символов
void calculate_frequencies(const char* filename, int* frequencies, uint64_t* file_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    memset(frequencies, 0, 256 * sizeof(int));
    
    int ch;
    *file_size = 0;
    while ((ch = fgetc(file)) != EOF) {
        frequencies[ch]++;
        (*file_size)++;
    }
    
    fclose(file);
}

// Функция сравнения для сортировки узлов (при совпадении частот сортируем по символам)
int compare_nodes(const void* a, const void* b) {
    const HuffmanNode* node1 = *(const HuffmanNode**)a;
    const HuffmanNode* node2 = *(const HuffmanNode**)b;
    
    if (node1->frequency != node2->frequency) {
        return node1->frequency - node2->frequency;
    }
    return node1->symbol - node2->symbol;
}

// Построение дерева Хаффмана
HuffmanNode* build_huffman_tree(int* frequencies) {
    HuffmanNode* nodes[256];
    int node_count = 0;

    // Создаем узлы для символов с ненулевой частотой
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            nodes[node_count++] = create_node((unsigned char)i, frequencies[i]);
        }
    }

    if (node_count == 0) return NULL;

    // Строим дерево
    while (node_count > 1) {
        // Сортируем узлы по частоте (при равенстве - по символу)
        qsort(nodes, node_count, sizeof(HuffmanNode*), compare_nodes);
        
        // Берем два узла с наименьшей частотой
        HuffmanNode* left = nodes[0];
        HuffmanNode* right = nodes[1];
        
        // Создаем родительский узел
        HuffmanNode* parent = create_node(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        
        // Обновляем массив узлов
        nodes[0] = parent;
        for (int i = 1; i < node_count - 1; i++) {
            nodes[i] = nodes[i + 1];
        }
        node_count--;
    }

    return nodes[0];
}

// Построение кодов Хаффмана
void build_codes_recursive(HuffmanNode* node, HuffmanCode* codes, uint32_t code, uint8_t depth) {
    if (node == NULL) return;

    if (node->left == NULL && node->right == NULL) {
        codes[node->symbol].code = code;
        codes[node->symbol].length = depth;
        return;
    }

    build_codes_recursive(node->left, codes, (code << 1) | 0, depth + 1);
    build_codes_recursive(node->right, codes, (code << 1) | 1, depth + 1);
}

void build_codes(HuffmanNode* root, HuffmanCode* codes) {
    for (int i = 0; i < 256; i++) {
        codes[i].code = 0;
        codes[i].length = 0;
    }
    build_codes_recursive(root, codes, 0, 0);
}

// Подсчет количества листьев в дереве
uint16_t count_tree_leaves(HuffmanNode* node) {
    if (node == NULL) return 0;
    if (node->left == NULL && node->right == NULL) return 1;
    return count_tree_leaves(node->left) + count_tree_leaves(node->right);
}

// Сериализация дерева в файл (префиксный обход)
void write_tree(FILE* file, HuffmanNode* node) {
    if (node == NULL) return;

    if (node->left == NULL && node->right == NULL) {
        fputc(1, file); // Маркер листа
        fputc(node->symbol, file);
    } else {
        fputc(0, file); // Маркер внутреннего узла
        write_tree(file, node->left);
        write_tree(file, node->right);
    }
}

// Десериализация дерева из файла
HuffmanNode* read_tree(FILE* file) {
    int marker = fgetc(file);
    if (marker == EOF) return NULL;

    if (marker == 1) {
        int symbol = fgetc(file);
        if (symbol == EOF) return NULL;
        return create_node((unsigned char)symbol, 0);
    } else {
        HuffmanNode* node = create_node(0, 0);
        node->left = read_tree(file);
        node->right = read_tree(file);
        return node;
    }
}

// Основная функция сжатия
int huffman_compress(const char* input_file, const char* output_file) {
    int frequencies[256];
    uint64_t file_size;
    
    calculate_frequencies(input_file, frequencies, &file_size);
    
    HuffmanNode* root = build_huffman_tree(frequencies);
    if (root == NULL) {
        fprintf(stderr, "Error building Huffman tree\n");
        return -1;
    }

    HuffmanCode codes[256];
    build_codes(root, codes);

    FILE* input = fopen(input_file, "rb");
    FILE* output = fopen(output_file, "wb");
    if (!input || !output) {
        perror("Failed to open files");
        free_huffman_tree(root);
        return -1;
    }

    // Записываем заголовок
    HuffmanHeader header;
    memcpy(header.signature, SEROSA_SIGNATURE, 6);
    header.major_version = 2;
    header.minor_version = 0;
    header.compression_no_context = HUFFMAN_ALGORITHM_CODE;
    header.compression_with_context = 0;
    header.error_protection = 0;
    header.original_size = file_size;
    header.tree_leaf_count = count_tree_leaves(root);
    memset(header.reserved, 0, 3);
    
    // Пока неизвестно compressed_size_bits, запишем позже
    long compressed_size_pos = sizeof(header) - sizeof(uint64_t);
    fwrite(&header, sizeof(header), 1, output);
    
    // Записываем дерево
    write_tree(output, root);

    // Сжимаем данные
    uint8_t current_byte = 0;
    uint8_t bit_count = 0;
    uint64_t compressed_bits = 0;
    uint64_t total_bits_written = 0;

    fseek(input, 0, SEEK_SET);
    int ch;
    while ((ch = fgetc(input)) != EOF) {
        HuffmanCode code = codes[ch];
        
        for (int i = 0; i < code.length; i++) {
            current_byte = (current_byte << 1) | ((code.code >> (code.length - 1 - i)) & 1);
            bit_count++;
            compressed_bits++;

            if (bit_count == 8) {
                fputc(current_byte, output);
                current_byte = 0;
                bit_count = 0;
                total_bits_written += 8;
            }
        }
    }

    // Дописываем оставшиеся биты
    if (bit_count > 0) {
        current_byte <<= (8 - bit_count);
        fputc(current_byte, output);
        total_bits_written += bit_count;
    }

    // Обновляем заголовок с реальным размером
    header.compressed_size_bits = compressed_bits;
    fseek(output, compressed_size_pos, SEEK_SET);
    fwrite(&header.compressed_size_bits, sizeof(uint64_t), 1, output);

    fclose(input);
    fclose(output);
    free_huffman_tree(root);

    // Анализ эффективности сжатия
    printf("Compression completed:\n");
    printf("  Original size: %lu bytes\n", (unsigned long)file_size);
    printf("  Compressed size: %lu bytes\n", (unsigned long)(total_bits_written / 8 + sizeof(header) + 2 * header.tree_leaf_count));
    printf("  Compression ratio: %.2f%%\n", (1.0 - (double)(total_bits_written / 8 + sizeof(header) + 2 * header.tree_leaf_count) / file_size) * 100);
    
    return 0;
}

// Основная функция распаковки
int huffman_decompress(const char* input_file, const char* output_file) {
    FILE* input = fopen(input_file, "rb");
    FILE* output = fopen(output_file, "wb");
    
    if (!input || !output) {
        perror("Failed to open files");
        return -1;
    }

    // Читаем заголовок
    HuffmanHeader header;
    if (fread(&header, sizeof(header), 1, input) != 1) {
        fprintf(stderr, "Error reading header\n");
        fclose(input);
        fclose(output);
        return -1;
    }

    // Проверяем сигнатуру
    if (memcmp(header.signature, SEROSA_SIGNATURE, 6) != 0) {
        fprintf(stderr, "Invalid file signature\n");
        fclose(input);
        fclose(output);
        return -1;
    }

    // Проверяем алгоритм
    if (header.compression_no_context != HUFFMAN_ALGORITHM_CODE) {
        fprintf(stderr, "Unsupported algorithm code: %d\n", header.compression_no_context);
        fclose(input);
        fclose(output);
        return -1;
    }

    // Восстанавливаем дерево
    HuffmanNode* root = read_tree(input);
    if (root == NULL) {
        fprintf(stderr, "Error reading Huffman tree\n");
        fclose(input);
        fclose(output);
        return -1;
    }

    // Распаковываем данные
    HuffmanNode* current = root;
    uint64_t bits_processed = 0;
    int byte;
    
    while ((byte = fgetc(input)) != EOF && bits_processed < header.compressed_size_bits) {
        for (int i = 7; i >= 0 && bits_processed < header.compressed_size_bits; i--) {
            int bit = (byte >> i) & 1;
            
            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }

            if (current->left == NULL && current->right == NULL) {
                fputc(current->symbol, output);
                current = root;
            }
            
            bits_processed++;
        }
    }

    fclose(input);
    fclose(output);
    free_huffman_tree(root);

    printf("Decompression completed successfully\n");
    return 0;
}

// Функция для расчета количества информации (для Л2.№1)
double calculate_information_bits(int* frequencies, uint64_t file_size) {
    double total_information = 0.0;
    
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            double probability = (double)frequencies[i] / file_size;
            double information = -log2(probability);
            total_information += frequencies[i] * information;
        }
    }
    
    return total_information;
}

// Функция анализа файла (для Л2.№1)
void analyze_file(const char* filename) {
    int frequencies[256];
    uint64_t file_size;
    
    calculate_frequencies(filename, frequencies, &file_size);
    
    if (file_size == 0) {
        printf("File is empty\n");
        return;
    }
    
    printf("=== File Analysis (%s) ===\n", filename);
    printf("File size: %lu bytes (%lu bits)\n", (unsigned long)file_size, (unsigned long)file_size * 8);
    
    // Сортируем символы по частоте
    typedef struct {
        unsigned char symbol;
        int frequency;
        double probability;
        double information;
    } SymbolInfo;
    
    SymbolInfo symbols[256];
    int symbol_count = 0;
    
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            symbols[symbol_count].symbol = (unsigned char)i;
            symbols[symbol_count].frequency = frequencies[i];
            symbols[symbol_count].probability = (double)frequencies[i] / file_size;
            symbols[symbol_count].information = -log2(symbols[symbol_count].probability);
            symbol_count++;
        }
    }
    
    // Сортировка по убыванию частоты
    for (int i = 0; i < symbol_count - 1; i++) {
        for (int j = i + 1; j < symbol_count; j++) {
            if (symbols[i].frequency < symbols[j].frequency) {
                SymbolInfo temp = symbols[i];
                symbols[i] = symbols[j];
                symbols[j] = temp;
            }
        }
    }
    
    // Вывод таблицы (первые 20 самых частых символов)
    printf("\nTop 20 symbols by frequency:\n");
    printf("Hex  Freq    Probability  Information(bits)\n");
    printf("--------------------------------------------\n");
    for (int i = 0; i < (symbol_count < 20 ? symbol_count : 20); i++) {
        printf("0x%02X %6d %12.6f %12.6f\n", 
               symbols[i].symbol, symbols[i].frequency, 
               symbols[i].probability, symbols[i].information);
    }
    
    // Расчет суммарной информации
    double total_information_bits = calculate_information_bits(frequencies, file_size);
    double total_information_bytes = total_information_bits / 8.0;
    
    printf("\nInformation Analysis:\n");
    printf("Total information I_BP(Q) [bits]: %.2f\n", total_information_bits);
    printf("Fractional part {I_BP(Q)}: %.2e\n", total_information_bits - floor(total_information_bits));
    printf("Total information I_BP(Q) [bytes]: %.2f\n", total_information_bytes);
    
    // Оценки снизу
    uint64_t E = (uint64_t)ceil(total_information_bytes);
    uint64_t G64 = E + 256 * 8;
    uint64_t G8 = E + 256 * 1;
    
    printf("\nLower bound estimates:\n");
    printf("E [bytes]  (compressed data only): %lu\n", E);
    printf("G64 [bytes] (with 64-bit freq table): %lu\n", G64);
    printf("G8 [bytes]  (with 8-bit freq table): %lu\n", G8);
    printf("Original file size [bytes]: %lu\n", (unsigned long)file_size);
    
    // Сравнение
    printf("\nComparison:\n");
    printf("Compression beneficial: %s\n", (G8 < file_size) ? "YES" : "NO");
    printf("Frequency normalization useful: %s\n", (G8 < G64) ? "YES" : "NO");
}