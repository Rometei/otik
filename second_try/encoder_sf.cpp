#include "common.h"
#include "shannon_fano.h"
#include "archive_format.h"
#include "frequency.h"
#include "bitstream.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
        return 1;
    }
    
    // Чтение входного файла
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open input file: " << argv[1] << std::endl;
        return 1;
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)), 
                             std::istreambuf_iterator<char>());
    input.close();
    
    if (data.empty()) {
        std::cerr << "Input file is empty" << std::endl;
        return 1;
    }
    
    // Анализ и выбор оптимальной разрядности
    auto freqs = FrequencyAnalyzer::calculateFrequencies(data);
    int bestBits = 8; // По умолчанию используем 8 бит
    
    // Простой подбор лучшей разрядности
    std::vector<int> bitOptions = {64, 32, 8, 4};
    uint64_t bestSize = UINT64_MAX;
    
    for (int bits : bitOptions) {
        try {
            auto normFreqs = FrequencyAnalyzer::normalizeFrequencies(freqs, bits);
            
            // Для оценки размера используем Шеннона-Фано
            ShannonFanoEncoder encoder(normFreqs);
            auto codes = encoder.getCodes();
            
            uint64_t compressedBits = 0;
            for (size_t i = 0; i < freqs.size(); i++) {
                if (freqs[i] > 0) {
                    auto it = codes.find(static_cast<uint8_t>(i));
                    if (it != codes.end()) {
                        compressedBits += freqs[i] * it->second.size();
                    }
                }
            }
            
            uint64_t totalSize = (compressedBits + 7) / 8 + 32 * bits;
            
            if (totalSize < bestSize) {
                bestSize = totalSize;
                bestBits = bits;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error with bits=" << bits << ": " << e.what() << std::endl;
        }
    }
    
    // Кодирование с выбранной разрядностью
    auto normFreqs = FrequencyAnalyzer::normalizeFrequencies(freqs, bestBits);
    ShannonFanoEncoder encoder(normFreqs);
    
    // Временный поток для измерения размера сжатых данных
    std::stringstream tempStream;
    BitOutputStream tempOut(tempStream);
    encoder.encodeData(data, tempOut);
    tempOut.flush();
    
    std::string compressedData = tempStream.str();
    uint64_t compressedSize = compressedData.size();
    
    // Запись архива
    std::ofstream output(argv[2], std::ios::binary);
    if (!output) {
        std::cerr << "Cannot create output file: " << argv[2] << std::endl;
        return 1;
    }
    
    ArchiveHeader header;
    header.signature = Common::SIGNATURE;
    header.version = Common::VERSION_3;  // Версия для Шеннона-Фано
    header.algorithm = Common::ALGO_SHANNON_FANO;
    header.frequencyBits = bestBits;
    header.reserved = 0;
    header.originalSize = data.size();
    header.compressedSize = compressedSize;
    
    ArchiveWriter::writeHeader(output, header);
    ArchiveWriter::writeFrequencies(output, normFreqs, bestBits);
    output.write(compressedData.c_str(), compressedSize);
    
    output.close();
    
    double ratio = (compressedSize * 100.0) / data.size();
    std::cout << "Shannon-Fano compression completed: " << data.size() << " -> " << compressedSize 
              << " bytes (" << ratio << "%)" << std::endl;
    std::cout << "Frequency bits: " << bestBits << std::endl;
    
    return 0;
}