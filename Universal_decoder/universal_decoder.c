#include "universal_decoder.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Структуры для дерева Хаффмана
typedef struct HuffmanNode {
    unsigned char symbol;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

// Создание узла дерева
HuffmanNode* create_node(unsigned char symbol) {
    HuffmanNode* node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
    node->symbol = symbol;
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

// Функция для чтения заголовка в совместимом формате
int read_compatible_header(FILE* input, BaseHeader* base_header, int* format_type) {
    // Сохраняем текущую позицию
    long start_pos = ftell(input);
    
    // Пытаемся прочитать базовый заголовок нового формата
    if (fread(base_header, sizeof(BaseHeader), 1, input) == 1) {
        if (memcmp(base_header->signature, SEROSA_SIGNATURE, 6) == 0) {
            *format_type = 1; // Новый формат
            return 0;
        }
    }
    
    // Возвращаемся и пробуем старый формат из Л3.№3
    fseek(input, start_pos, SEEK_SET);
    
    unsigned char signature[6];
    uint16_t major_version, minor_version;
    unsigned char compression_no_context;
    unsigned char compression_with_context;
    unsigned char error_protection;
    uint64_t original_size;
    unsigned char reserved[5];
    
    if (fread(signature, 1, 6, input) != 6) return -1;
    if (fread(&major_version, sizeof(uint16_t), 1, input) != 1) return -1;
    if (fread(&minor_version, sizeof(uint16_t), 1, input) != 1) return -1;
    if (fread(&compression_no_context, 1, 1, input) != 1) return -1;
    if (fread(&compression_with_context, 1, 1, input) != 1) return -1;
    if (fread(&error_protection, 1, 1, input) != 1) return -1;
    if (fread(&original_size, sizeof(uint64_t), 1, input) != 1) return -1;
    if (fread(reserved, 1, 5, input) != 5) return -1;
    
    if (memcmp(signature, SEROSA_SIGNATURE, 6) != 0) {
        return -1;
    }
    
    // Конвертируем в базовый заголовок
    memcpy(base_header->signature, signature, 6);
    base_header->version = (major_version << 8) | minor_version;
    base_header->algorithm = compression_no_context;
    base_header->original_size = original_size;
    
    *format_type = 2; // Старый формат из Л3.№3
    return 0;
}

// Чтение дерева Хаффмана из файла
HuffmanNode* read_huffman_tree(FILE* file) {
    int marker = fgetc(file);
    if (marker == EOF) return NULL;

    if (marker == 1) {
        // Лист
        int symbol = fgetc(file);
        if (symbol == EOF) return NULL;
        return create_node((unsigned char)symbol);
    } else {
        // Внутренний узел
        HuffmanNode* node = create_node(0);
        node->left = read_huffman_tree(file);
        node->right = read_huffman_tree(file);
        return node;
    }
}

// Декодер без сжатия
int decode_no_compression(FILE* input, FILE* output, BaseHeader* header, int format_type) {
    printf("Using algorithm: No compression\n");
    
    if (format_type == 2) {
        // Старый формат из Л3.№3 - данные идут сразу после заголовка
        // (мы уже прочитали весь заголовок в read_compatible_header)
    } else if (format_type == 1) {
        // Новый формат - пропускаем оставшуюся часть заголовка
        unsigned char reserved[10];
        if (fread(reserved, 1, 10, input) != 10) {
            fprintf(stderr, "Error reading reserved bytes\n");
            return -1;
        }
    }
    
    // Копируем данные
    unsigned char buffer[4096];
    uint64_t remaining = header->original_size;
    uint64_t total_read = 0;
    
    while (remaining > 0) {
        size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        size_t bytes_read = fread(buffer, 1, to_read, input);
        if (bytes_read == 0) {
            break;
        }
        fwrite(buffer, 1, bytes_read, output);
        remaining -= bytes_read;
        total_read += bytes_read;
    }
    
    if (remaining > 0) {
        fprintf(stderr, "Error: incomplete data (expected %lu, got %lu)\n", 
                (unsigned long)header->original_size, (unsigned long)total_read);
        return -1;
    }
    
    printf("Successfully decompressed %lu bytes\n", (unsigned long)total_read);
    return 0;
}

// Декодер Хаффмана
int decode_huffman(FILE* input, FILE* output, BaseHeader* base_header, int format_type) {
    printf("Using algorithm: Huffman coding\n");
    
    uint64_t compressed_size_bits;
    uint16_t tree_leaf_count;
    
    if (format_type == 1) {
        // Новый формат
        HuffmanHeader header;
        memcpy(&header.base, base_header, sizeof(BaseHeader));
        size_t huffman_specific_size = sizeof(HuffmanHeader) - sizeof(BaseHeader);
        if (fread(((unsigned char*)&header) + sizeof(BaseHeader), huffman_specific_size, 1, input) != 1) {
            fprintf(stderr, "Error reading Huffman header data\n");
            return -1;
        }
        compressed_size_bits = header.compressed_size_bits;
        tree_leaf_count = header.tree_leaf_count;
    } else {
        // Старый формат - читаем дополнительные поля
        if (fread(&compressed_size_bits, sizeof(uint64_t), 1, input) != 1) return -1;
        if (fread(&tree_leaf_count, sizeof(uint16_t), 1, input) != 1) return -1;
        
        // Пропускаем reserved bytes
        unsigned char reserved[3];
        if (fread(reserved, 1, 3, input) != 3) return -1;
    }
    
    printf("Original size: %lu bytes\n", (unsigned long)base_header->original_size);
    printf("Compressed bits: %lu\n", (unsigned long)compressed_size_bits);
    printf("Tree leaves: %u\n", tree_leaf_count);

    // Читаем дерево Хаффмана
    HuffmanNode* root = read_huffman_tree(input);
    if (root == NULL) {
        fprintf(stderr, "Error reading Huffman tree\n");
        return -1;
    }

    // Распаковываем данные
    HuffmanNode* current = root;
    uint64_t bits_processed = 0;
    int byte;
    uint64_t bytes_written = 0;
    
    while ((byte = fgetc(input)) != EOF && bits_processed < compressed_size_bits) {
        for (int i = 7; i >= 0 && bits_processed < compressed_size_bits; i--) {
            int bit = (byte >> i) & 1;
            
            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }

            if (current->left == NULL && current->right == NULL) {
                fputc(current->symbol, output);
                bytes_written++;
                current = root;
                
                if (bytes_written > base_header->original_size) {
                    fprintf(stderr, "Error: decompressed data exceeds original size\n");
                    free_huffman_tree(root);
                    return -1;
                }
            }
            
            bits_processed++;
        }
    }

    if (bytes_written != base_header->original_size) {
        fprintf(stderr, "Warning: decompressed size mismatch (expected %lu, got %lu)\n",
                (unsigned long)base_header->original_size, (unsigned long)bytes_written);
    } else {
        printf("Successfully decompressed %lu bytes\n", (unsigned long)bytes_written);
    }

    free_huffman_tree(root);
    return 0;
}

// Определение формата и выбор декодера
int detect_format_and_decode(FILE* input, FILE* output) {
    BaseHeader base_header;
    int format_type;
    
    // Читаем заголовок в совместимом формате
    if (read_compatible_header(input, &base_header, &format_type) != 0) {
        fprintf(stderr, "Invalid file signature. Expected 'SEROSA'\n");
        return -1;
    }

    uint8_t major_version = (base_header.version >> 8) & 0xFF;
    uint8_t minor_version = base_header.version & 0xFF;
    
    printf("Detected format: SEROSA v%d.%d, Algorithm: %d, Format type: %s\n",
           major_version, minor_version, base_header.algorithm,
           format_type == 1 ? "new" : "old");

    // Выбираем соответствующий декодер
    switch (base_header.algorithm) {
        case ALGORITHM_NO_COMPRESSION:
            return decode_no_compression(input, output, &base_header, format_type);
            
        case ALGORITHM_HUFFMAN:
            return decode_huffman(input, output, &base_header, format_type);
            
        default:
            fprintf(stderr, "Unsupported algorithm code: %d\n", base_header.algorithm);
            return -1;
    }
}

// Основная функция универсального декодера
int universal_decode(const char* input_path, const char* output_path) {
    FILE* input = fopen(input_path, "rb");
    if (!input) {
        perror("Failed to open input file");
        return -1;
    }

    FILE* output = fopen(output_path, "wb");
    if (!output) {
        perror("Failed to open output file");
        fclose(input);
        return -1;
    }

    printf("Decoding: %s -> %s\n", input_path, output_path);
    
    int result = detect_format_and_decode(input, output);
    
    fclose(input);
    fclose(output);
    
    if (result == 0) {
        printf("Decoding completed successfully\n");
    } else {
        printf("Decoding failed\n");
        remove(output_path);
    }
    
    return result;
}

void print_supported_formats() {
    printf("Supported SEROSA formats:\n");
    printf("  - Old format (Л3.№3): No compression\n");
    printf("  - New format (Л4.№1): No compression and Huffman coding\n");
    printf("  - All versions with proper 'SEROSA' signature\n");
}