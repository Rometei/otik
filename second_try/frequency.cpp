#include "frequency.h"
#include "huffman.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>

std::vector<uint64_t> FrequencyAnalyzer::calculateFrequencies(const std::vector<uint8_t>& data) {
    std::vector<uint64_t> freqs(Common::ALPHABET_SIZE, 0);
    for (uint8_t byte : data) {
        freqs[byte]++;
    }
    return freqs;
}

std::vector<uint64_t> FrequencyNormalizer::normalizeToBits(const std::vector<uint64_t>& freqs, int targetBits) {
    if (targetBits >= 64) return freqs;
    
    uint64_t maxValue = (1ULL << targetBits) - 1;
    return normalizeToMaxValue(freqs, maxValue);
}

std::vector<uint64_t> FrequencyNormalizer::normalizeToMaxValue(const std::vector<uint64_t>& freqs, uint64_t targetMax) {
    std::vector<uint64_t> normalized(freqs.size(), 0);
    uint64_t total = std::accumulate(freqs.begin(), freqs.end(), 0ULL);
    
    if (total == 0) return normalized;
    
    // Гарантируем, что ненулевые частоты остаются ненулевыми
    uint64_t nonZeroCount = 0;
    for (uint64_t f : freqs) {
        if (f > 0) nonZeroCount++;
    }
    
    if (nonZeroCount > targetMax) {
        // Слишком много ненулевых частот - используем пропорциональное масштабирование
        double scale = static_cast<double>(targetMax) / total;
        for (size_t i = 0; i < freqs.size(); i++) {
            uint64_t scaledValue = static_cast<uint64_t>(freqs[i] * scale);
            normalized[i] = (scaledValue > 0) ? scaledValue : 1;
        }
    } else {
        // Распределяем targetMax пропорционально с гарантией минимум 1 для ненулевых
        uint64_t reserved = nonZeroCount;
        uint64_t remaining = targetMax - reserved;
        
        if (total > nonZeroCount) {
            double scale = static_cast<double>(remaining) / (total - nonZeroCount);
            
            for (size_t i = 0; i < freqs.size(); i++) {
                if (freqs[i] > 0) {
                    normalized[i] = 1 + static_cast<uint64_t>((freqs[i] - 1) * scale);
                }
            }
        } else {
            // Все символы встречаются одинаково - равномерное распределение
            for (size_t i = 0; i < freqs.size(); i++) {
                if (freqs[i] > 0) {
                    normalized[i] = 1;
                }
            }
            // Распределяем оставшееся равномерно
            uint64_t extra = targetMax - nonZeroCount;
            for (size_t i = 0; i < freqs.size() && extra > 0; i++) {
                if (freqs[i] > 0) {
                    normalized[i]++;
                    extra--;
                }
            }
        }
    }
    
    // Корректируем сумму до targetMax
    uint64_t currentSum = std::accumulate(normalized.begin(), normalized.end(), 0ULL);
    if (currentSum != targetMax) {
        int64_t diff = static_cast<int64_t>(targetMax) - static_cast<int64_t>(currentSum);
        distributeRemainder(normalized, freqs, diff);
    }
    
    return normalized;
}

std::vector<uint64_t> FrequencyAnalyzer::normalizeFrequencies(const std::vector<uint64_t>& freqs, int bits) {
    return FrequencyNormalizer::normalizeToBits(freqs, bits);
}

uint64_t FrequencyAnalyzer::calculateCompressedSize(const std::vector<uint64_t>& origFreqs, 
                                                  const std::vector<uint64_t>& normFreqs) {
    // Проверяем, есть ли ненулевые частоты
    bool allZero = true;
    for (uint64_t f : normFreqs) {
        if (f > 0) {
            allZero = false;
            break;
        }
    }
    
    if (allZero) {
        return origFreqs.size() > 0 ? origFreqs.size() * 8 : 0; // Минимальная оценка
    }
    
    HuffmanEncoder encoder(normFreqs);
    auto codes = encoder.getCodes();
    
    uint64_t totalBits = 0;
    for (size_t i = 0; i < origFreqs.size(); i++) {
        if (origFreqs[i] > 0) {
            auto it = codes.find(static_cast<uint8_t>(i));
            if (it != codes.end()) {
                totalBits += origFreqs[i] * it->second.size();
            } else {
                // Если символ не найден в кодах, используем максимальную длину
                totalBits += origFreqs[i] * 256; // Консервативная оценка
            }
        }
    }
    return totalBits;
}

void FrequencyAnalyzer::analyzeFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), 
                             std::istreambuf_iterator<char>());
    file.close();
    
    if (data.empty()) {
        std::cout << "File: " << filename << " (size: 0 bytes) - empty file" << std::endl;
        return;
    }
    
    auto origFreqs = calculateFrequencies(data);
    uint64_t fileSize = data.size();
    
    std::cout << "File: " << filename << " (size: " << fileSize << " bytes)" << std::endl;
    std::cout << "Bits\tEB (bytes)\tGB (bytes)\tOverhead" << std::endl;
    
    std::vector<int> bitOptions = {64, 32, 8, 4};
    uint64_t bestGB = UINT64_MAX;
    int bestBits = 0;
    
    for (int bits : bitOptions) {
        try {
            auto normFreqs = normalizeFrequencies(origFreqs, bits);
            uint64_t compressedBits = calculateCompressedSize(origFreqs, normFreqs);
            uint64_t EB = (compressedBits + 7) / 8;
            uint64_t GB = EB + 32 * bits; // 256 * bits/8 = 32*bits
            double overhead = (GB * 100.0) / fileSize - 100;
            
            std::cout << bits << "\t" << EB << "\t\t" << GB << "\t\t" << overhead << "%" << std::endl;
            
            if (GB < bestGB) {
                bestGB = GB;
                bestBits = bits;
            }
        } catch (const std::exception& e) {
            std::cout << bits << "\tERROR: " << e.what() << std::endl;
        }
    }
    
    std::cout << "Best bits: " << bestBits << " (GB = " << bestGB << " bytes)" << std::endl << std::endl;
}

void FrequencyNormalizer::distributeRemainder(std::vector<uint64_t>& normalized, 
                                            const std::vector<uint64_t>& freqs, int64_t remainder) {
    if (remainder == 0) return;
    
    // Сортируем индексы по убыванию исходных частот
    std::vector<size_t> indices(freqs.size());
    for (size_t i = 0; i < indices.size(); i++) indices[i] = i;
    
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return freqs[a] > freqs[b];
    });
    
    if (remainder > 0) {
        // Добавляем к самым частым символам
        for (size_t i = 0; i < static_cast<size_t>(remainder); i++) {
            normalized[indices[i % indices.size()]]++;
        }
    } else {
        // Убираем у самых частых символов, но не ниже 1
        size_t absRemainder = static_cast<size_t>(-remainder);
        for (size_t i = 0; i < absRemainder; i++) {
            size_t idx = indices[i % indices.size()];
            if (normalized[idx] > 1) {
                normalized[idx]--;
            } else {
                // Если нельзя уменьшить, пропускаем этот символ
                absRemainder++;
                if (absRemainder > indices.size() * 2) break; // Защита от бесконечного цикла
            }
        }
    }
}