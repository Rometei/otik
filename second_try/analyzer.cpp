// analyzer.cpp - анализатор файлов
#include "frequency.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file1> [file2] ..." << std::endl;
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        FrequencyAnalyzer::analyzeFile(argv[i]);
    }
    
    return 0;
}