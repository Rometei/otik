#include <stdio.h>
#include <string.h>
#include "huffman.h"

void print_usage(const char* program_name) {
    printf("Usage: %s <command> <input_file> [output_file]\n", program_name);
    printf("Commands:\n");
    printf("  -c    Compress file\n");
    printf("  -d    Decompress file\n");
    printf("  -a    Analyze file (L2.No1)\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-c") == 0) {
        if (argc != 4) {
            printf("Error: compress requires input and output files\n");
            print_usage(argv[0]);
            return 1;
        }
        return huffman_compress(argv[2], argv[3]);
    } else if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            printf("Error: decompress requires input and output files\n");
            print_usage(argv[0]);
            return 1;
        }
        return huffman_decompress(argv[2], argv[3]);
    } else if (strcmp(argv[1], "-a") == 0) {
        if (argc != 3) {
            printf("Error: analyze requires input file only\n");
            print_usage(argv[0]);
            return 1;
        }
        analyze_file(argv[2]);
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}