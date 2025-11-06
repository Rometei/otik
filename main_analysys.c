#include <stdio.h>
#include <string.h>
#include "huffman_analysys.h"

void print_usage(const char* program_name) {
    printf("Usage: %s <command> [filename]\n", program_name);
    printf("Commands:\n");
    printf("  -a <file>    Analyze file for optimal bit width\n");
    printf("  -t           Run tests with known patterns\n");
    printf("  -h           Show this help\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 3) {
            printf("Error: analysis requires filename\n");
            print_usage(argv[0]);
            return 1;
        }
        analyze_file_optimal_bits(argv[2]);
    } else if (strcmp(argv[1], "-t") == 0) {
        test_with_known_files();
    } else if (strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}