#include "huffman_analysys.h"

// Создание узла дерева Хаффмана
HuffmanNode* create_node(unsigned char symbol, uint64_t frequency) {
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

// Подсчет частот символов в файле
uint64_t* calculate_frequencies(const char* filename, uint64_t* file_size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    uint64_t* frequencies = (uint64_t*)calloc(256, sizeof(uint64_t));
    if (!frequencies) {
        fclose(file);
        return NULL;
    }
    
    int ch;
    *file_size = 0;
    while ((ch = fgetc(file)) != EOF) {
        frequencies[ch]++;
        (*file_size)++;
    }
    
    fclose(file);
    return frequencies;
}

// Функция сравнения для сортировки узлов
int compare_nodes(const void* a, const void* b) {
    const HuffmanNode* node1 = *(const HuffmanNode**)a;
    const HuffmanNode* node2 = *(const HuffmanNode**)b;
    
    if (node1->frequency != node2->frequency) {
        return (node1->frequency > node2->frequency) ? 1 : -1;
    }
    return node1->symbol - node2->symbol;
}

// Построение дерева Хаффмана
HuffmanNode* build_huffman_tree(uint64_t* frequencies) {
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

// Рекурсивное построение кодов Хаффмана
void build_huffman_codes_recursive(HuffmanNode* node, HuffmanCode* codes, uint32_t code, uint8_t depth) {
    if (node == NULL) return;

    if (node->left == NULL && node->right == NULL) {
        codes[node->symbol].code = code;
        codes[node->symbol].length = depth;
        return;
    }

    build_huffman_codes_recursive(node->left, codes, (code << 1) | 0, depth + 1);
    build_huffman_codes_recursive(node->right, codes, (code << 1) | 1, depth + 1);
}

void build_huffman_codes(HuffmanNode* root, HuffmanCode* codes, uint32_t code, uint8_t depth) {
    for (int i = 0; i < 256; i++) {
        codes[i].code = 0;
        codes[i].length = 0;
    }
    build_huffman_codes_recursive(root, codes, code, depth);
}

// Расчет размера сжатых данных
uint64_t calculate_compressed_size(uint64_t* frequencies, HuffmanCode* codes) {
    uint64_t total_bits = 0;
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0 && codes[i].length > 0) {
            total_bits += frequencies[i] * codes[i].length;
        }
    }
    return total_bits;
}

// Нормализация частот для заданной разрядности
void normalize_frequencies(uint64_t* src_freqs, uint64_t* dst_freqs, int bits, uint64_t file_size) {
    if (bits == 64) {
        // Для 64 бит используем исходные частоты
        memcpy(dst_freqs, src_freqs, 256 * sizeof(uint64_t));
        return;
    }

    uint64_t max_value = (1ULL << bits) - 1;
    if (max_value == 0) max_value = 1; // Для bits=1

    // Находим максимальную частоту среди ненулевых символов
    uint64_t max_freq = 0;
    int non_zero_count = 0;
    
    for (int i = 0; i < 256; i++) {
        if (src_freqs[i] > 0) {
            non_zero_count++;
            if (src_freqs[i] > max_freq) {
                max_freq = src_freqs[i];
            }
        }
    }

    if (max_freq == 0) {
        memset(dst_freqs, 0, 256 * sizeof(uint64_t));
        return;
    }

    // Если максимальная частота уже в пределах диапазона, используем как есть
    if (max_freq <= max_value) {
        memcpy(dst_freqs, src_freqs, 256 * sizeof(uint64_t));
        return;
    }

    // Нормализуем частоты
    double scale_factor = (double)max_value / max_freq;
    
    for (int i = 0; i < 256; i++) {
        if (src_freqs[i] > 0) {
            uint64_t normalized = (uint64_t)round(src_freqs[i] * scale_factor);
            dst_freqs[i] = (normalized == 0) ? 1 : normalized; // Гарантируем минимум 1 для ненулевых
            if (dst_freqs[i] > max_value) {
                dst_freqs[i] = max_value;
            }
        } else {
            dst_freqs[i] = 0;
        }
    }

    // Для очень маленьких файлов дополнительная проверка
    if (bits < 8) {
        uint64_t sum = 0;
        for (int i = 0; i < 256; i++) {
            sum += dst_freqs[i];
        }
        if (sum == 0) {
            // Если все стали нулями, устанавливаем минимальные значения для ненулевых символов
            for (int i = 0; i < 256; i++) {
                if (src_freqs[i] > 0) {
                    dst_freqs[i] = 1;
                }
            }
        }
    }
}

// Основная функция анализа
void analyze_file_optimal_bits(const char* filename) {
    uint64_t file_size;
    uint64_t* original_freqs = calculate_frequencies(filename, &file_size);
    
    if (!original_freqs || file_size == 0) {
        printf("File is empty or cannot be read\n");
        free(original_freqs);
        return;
    }

    printf("=== Huffman Compression Analysis for: %s ===\n", filename);
    printf("Original file size: %lu bytes\n", (unsigned long)file_size);
    printf("\n");

    uint64_t best_GB = UINT64_MAX;
    int best_B = 0;
    uint64_t best_EB = 0;

    // Анализируем различные разрядности
    printf("%-8s %-12s %-12s %-12s\n", "Bits(B)", "EB(bytes)", "GB(bytes)", "Efficiency");
    printf("------------------------------------------------\n");

    for (int B = 1; B <= 64; B++) {
        uint64_t normalized_freqs[256] = {0};
        normalize_frequencies(original_freqs, normalized_freqs, B, file_size);

        // Строим дерево Хаффмана для нормализованных частот
        HuffmanNode* root = build_huffman_tree(normalized_freqs);
        if (root == NULL) {
            continue;
        }

        HuffmanCode codes[256];
        build_huffman_codes(root, codes, 0, 0);

        // Вычисляем размер сжатых данных
        uint64_t compressed_bits = calculate_compressed_size(normalized_freqs, codes);
        uint64_t EB = (compressed_bits + 7) / 8; // Округление вверх до байтов
        uint64_t GB = EB + 32 * B; // 256 * B/8 = 32 * B

        double efficiency = (1.0 - (double)GB / file_size) * 100;

        // Выводим результаты для ключевых разрядностей
        if (B == 64 || B == 32 || B == 8 || B == 4) {
            printf("%-8d %-12lu %-12lu %-11.2f%%\n", B, EB, GB, efficiency);
        }

        if (GB < best_GB) {
            best_GB = GB;
            best_B = B;
            best_EB = EB;
        }

        free_huffman_tree(root);
    }

    printf("\n");
    printf("Optimal bit width B*: %d bits\n", best_B);
    printf("Best compressed size EB: %lu bytes\n", best_EB);
    printf("Best total size GB: %lu bytes\n", best_GB);
    printf("Best efficiency: %.2f%%\n", (1.0 - (double)best_GB / file_size) * 100);

    // Сравнение с оценками из Л2.№1
    printf("\n=== Comparison with L2.No1 estimates ===\n");
    
    // Расчет энтропии для сравнения
    double entropy_bits = 0.0;
    for (int i = 0; i < 256; i++) {
        if (original_freqs[i] > 0) {
            double p = (double)original_freqs[i] / file_size;
            entropy_bits += p * (-log2(p));
        }
    }
    double entropy_bytes = entropy_bits * file_size / 8;
    uint64_t E_estimate = (uint64_t)ceil(entropy_bytes);
    uint64_t G64_estimate = E_estimate + 256 * 8;
    uint64_t G8_estimate = E_estimate + 256 * 1;

    printf("Theoretical entropy-based estimates:\n");
    printf("E (compressed only): %lu bytes\n", E_estimate);
    printf("G64 (with 64-bit freqs): %lu bytes\n", G64_estimate);
    printf("G8 (with 8-bit freqs): %lu bytes\n", G8_estimate);
    printf("Actual best GB: %lu bytes\n", best_GB);

    // Выводы
    printf("\n=== Conclusions ===\n");
    printf("1. Difference from L2.No1 estimates: %.2f%%\n", 
           ((double)best_GB - G8_estimate) * 100.0 / G8_estimate);
    printf("2. Optimal fixed bit width B**: %d bits\n", (best_B <= 8) ? 8 : 32);
    printf("3. Benefits of adaptive B* selection: %s\n", 
           (best_GB < G8_estimate * 0.95) ? "SIGNIFICANT" : "MINIMAL");
    printf("4. Recommended approach: %s\n",
           (file_size > 1000000) ? "Use adaptive B*" : "Use fixed B=8");

    free(original_freqs);
}

// Функция для проверки на тестовых файлах
void test_with_known_files() {
    printf("=== Testing with known patterns ===\n");
    
    // Создаем тестовые файлы
    FILE* test1 = fopen("test_uniform.bin", "wb");
    FILE* test2 = fopen("test_skewed.bin", "wb");
    
    if (test1 && test2) {
        // Тест 1: Равномерное распределение (4 разных байта)
        unsigned char data1[] = {0x00, 0x01, 0x02, 0x03};
        for (int i = 0; i < 256; i++) {
            fwrite(data1, 1, 4, test1);
        }
        
        // Тест 2: Скошенное распределение (один байт доминирует)
        for (int i = 0; i < 1000; i++) {
            fputc(0x00, test2);
            if (i % 10 == 0) fputc(0x01, test2);
        }
        
        fclose(test1);
        fclose(test2);
        
        // Анализируем тестовые файлы
        analyze_file_optimal_bits("test_uniform.bin");
        printf("\n");
        analyze_file_optimal_bits("test_skewed.bin");
        
        // Удаляем временные файлы
        remove("test_uniform.bin");
        remove("test_skewed.bin");
    }
}