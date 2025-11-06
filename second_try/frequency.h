#pragma once
#include "common.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <string>

class FrequencyAnalyzer {
public:
    static std::vector<uint64_t> calculateFrequencies(const std::vector<uint8_t>& data);
    static std::vector<uint64_t> normalizeFrequencies(const std::vector<uint64_t>& freqs, int bits);
    
    static uint64_t calculateCompressedSize(const std::vector<uint64_t>& origFreqs, 
                                          const std::vector<uint64_t>& normFreqs);
    
    static void analyzeFile(const std::string& filename);
};

class FrequencyNormalizer {
public:
    static std::vector<uint64_t> normalizeToBits(const std::vector<uint64_t>& freqs, int targetBits);
    
private:
    static std::vector<uint64_t> normalizeToMaxValue(const std::vector<uint64_t>& freqs, uint64_t targetMax);
    static void distributeRemainder(std::vector<uint64_t>& normalized, 
                                  const std::vector<uint64_t>& freqs, int64_t remainder);
};