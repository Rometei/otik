#include "common.h"
#include "huffman.h"
#include "shannon_fano.h"
#include "frequency.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>

void compareAlgorithms(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), 
                             std::istreambuf_iterator<char>());
    file.close();
    
    if (data.empty()) {
        std::cout << "File is empty" << std::endl;
        return;
    }
    
    auto freqs = FrequencyAnalyzer::calculateFrequencies(data);
    uint64_t originalSize = data.size();
    
    std::cout << "File: " << filename << " (" << originalSize << " bytes)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << std::setw(15) << "Algorithm" 
              << std::setw(15) << "EB (bytes)" 
              << std::setw(15) << "Ratio (%)" 
              << std::setw(15) << "Bits/symbol" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Тестируем разные разрядности частот
    std::vector<int> bitOptions = {64, 32, 8, 4};
    
    for (int bits : bitOptions) {
        auto normFreqs = FrequencyAnalyzer::normalizeFrequencies(freqs, bits);
        
        // Хаффман
        try {
            HuffmanEncoder huffEncoder(normFreqs);
            auto huffCodes = huffEncoder.getCodes();
            
            uint64_t huffBits = 0;
            for (size_t i = 0; i < freqs.size(); i++) {
                if (freqs[i] > 0) {
                    auto it = huffCodes.find(static_cast<uint8_t>(i));
                    if (it != huffCodes.end()) {
                        huffBits += freqs[i] * it->second.size();
                    }
                }
            }
            
            uint64_t huffEB = (huffBits + 7) / 8;
            double huffRatio = (huffEB * 100.0) / originalSize;
            double avgBits = (huffBits * 1.0) / originalSize;
            
            std::cout << std::setw(15) << ("Huffman-" + std::to_string(bits))
                      << std::setw(15) << huffEB
                      << std::setw(15) << std::fixed << std::setprecision(2) << huffRatio
                      << std::setw(15) << std::setprecision(3) << avgBits << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Huffman-" << bits << " ERROR: " << e.what() << std::endl;
        }
        
        // Шеннон-Фано
        try {
            ShannonFanoEncoder sfEncoder(normFreqs);
            auto sfCodes = sfEncoder.getCodes();
            
            uint64_t sfBits = 0;
            for (size_t i = 0; i < freqs.size(); i++) {
                if (freqs[i] > 0) {
                    auto it = sfCodes.find(static_cast<uint8_t>(i));
                    if (it != sfCodes.end()) {
                        sfBits += freqs[i] * it->second.size();
                    }
                }
            }
            
            uint64_t sfEB = (sfBits + 7) / 8;
            double sfRatio = (sfEB * 100.0) / originalSize;
            double avgBits = (sfBits * 1.0) / originalSize;
            
            std::cout << std::setw(15) << ("ShannonFano-" + std::to_string(bits))
                      << std::setw(15) << sfEB
                      << std::setw(15) << std::fixed << std::setprecision(2) << sfRatio
                      << std::setw(15) << std::setprecision(3) << avgBits << std::endl;
        } catch (const std::exception& e) {
            std::cout << "ShannonFano-" << bits << " ERROR: " << e.what() << std::endl;
        }
        
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file1> [file2] ..." << std::endl;
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        compareAlgorithms(argv[i]);
    }
    
    return 0;
}