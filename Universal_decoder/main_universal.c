#include <stdio.h>
#include <string.h>
#include "universal_decoder.h"

void print_usage(const char* program_name) {
    printf("Universal SEROSA Archive Decoder\n");
    printf("Usage: %s <command> [arguments]\n", program_name);
    printf("Commands:\n");
    printf("  -d <input> <output>  Decode archive\n");
    printf("  -l                   List supported formats\n");
    printf("  -v                   Show version information\n");
}

void print_version() {
    printf("Universal SEROSA Decoder v2.0\n");
    printf("Supports multiple format versions and compression algorithms\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            printf("Error: decode requires input and output files\n");
            print_usage(argv[0]);
            return 1;
        }
        return universal_decode(argv[2], argv[3]);
    } else if (strcmp(argv[1], "-l") == 0) {
        print_supported_formats();
        return 0;
    } else if (strcmp(argv[1], "-v") == 0) {
        print_version();
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}