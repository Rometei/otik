#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const unsigned char SIGNATURE[6] = {'S','E','R','O','S','A'};

#pragma pack(push, 1)
typedef struct {
    unsigned char signature[6];
    uint16_t version;
    uint16_t algorithm;
    uint64_t original_size;
    unsigned char reserved[10];
} OldArchiveHeader;
#pragma pack(pop)

// Улучшенный старый декодер с проверкой версии и алгоритма
int old_decoder_decode(const char* input_path, const char* output_path) {
    FILE* input = fopen(input_path, "rb");
    if (!input) {
        perror("Failed to open input file");
        return 1;
    }

    OldArchiveHeader header;
    if (fread(&header, sizeof(header), 1, input) != 1) {
        fprintf(stderr, "Error reading header\n");
        fclose(input);
        return 1;
    }

    // Проверяем сигнатуру
    if (memcmp(header.signature, SIGNATURE, 6) != 0) {
        fprintf(stderr, "Invalid file signature\n");
        fclose(input);
        return 1;
    }

    // Проверяем версию (старый декодер поддерживает только 1.0)
    if (header.version != 0x0100) {
        fprintf(stderr, "Unsupported format version: %d.%d\n", 
                header.version >> 8, header.version & 0xFF);
        fprintf(stderr, "This decoder only supports version 1.0\n");
        fprintf(stderr, "Please use the universal decoder for newer versions\n");
        fclose(input);
        return 1;
    }

    // Проверяем алгоритм (старый декодер поддерживает только без сжатия)
    if (header.algorithm != 0) {
        fprintf(stderr, "Unsupported algorithm: %d\n", header.algorithm);
        fprintf(stderr, "This decoder only supports no compression (algorithm 0)\n");
        fclose(input);
        return 1;
    }

    // Создаем выходной файл только после успешных проверок
    FILE* output = fopen(output_path, "wb");
    if (!output) {
        perror("Failed to create output file");
        fclose(input);
        return 1;
    }

    printf("Using old decoder (v1.0, algorithm 0)\n");
    
    // Копируем данные
    unsigned char buffer[4096];
    uint64_t total_read = 0;
    while (total_read < header.original_size) {
        size_t to_read = sizeof(buffer);
        if (header.original_size - total_read < to_read) {
            to_read = header.original_size - total_read;
        }
        size_t bytes_read = fread(buffer, 1, to_read, input);
        if (bytes_read == 0) break;
        fwrite(buffer, 1, bytes_read, output);
        total_read += bytes_read;
    }

    fclose(input);
    fclose(output);
    
    if (total_read == header.original_size) {
        printf("Decoding completed successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Error: incomplete data (expected %lu, got %lu)\n",
                (unsigned long)header.original_size, (unsigned long)total_read);
        return 1;
    }
}