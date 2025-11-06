#pragma once
#include "common.h"
#include "bitstream.h"
#include <vector>
#include <map>
#include <algorithm>
#include <functional>

struct SFNode {
    uint8_t symbol;
    uint64_t frequency;
    std::vector<bool> code;
    
    SFNode(uint8_t s, uint64_t f) : symbol(s), frequency(f) {}
};

class ShannonFanoEncoder {
public:
    ShannonFanoEncoder(const std::vector<uint64_t>& frequencies);
    std::map<uint8_t, std::vector<bool>> getCodes() const { return codes; }
    void encodeData(const std::vector<uint8_t>& data, BitOutputStream& out) const;
    
private:
    void buildCodes(const std::vector<SFNode>& nodes, int start, int end, uint64_t totalFreq);
    std::vector<bool> currentCode;
    std::map<uint8_t, std::vector<bool>> codes;
};

class ShannonFanoDecoder {
public:
    ShannonFanoDecoder(const std::vector<uint64_t>& frequencies);
    ~ShannonFanoDecoder(); // Добавляем объявление деструктора
    std::vector<uint8_t> decodeData(BitInputStream& in, size_t originalSize) const;
    
private:
    void buildTree(const std::vector<uint64_t>& frequencies);
    uint8_t decodeSymbol(BitInputStream& in) const;
    
    struct SFDecodeNode {
        uint8_t symbol;
        SFDecodeNode* left;
        SFDecodeNode* right;
        
        SFDecodeNode(uint8_t s) : symbol(s), left(nullptr), right(nullptr) {}
        SFDecodeNode() : symbol(0), left(nullptr), right(nullptr) {}
    };
    
    SFDecodeNode* root;
    void clearTree(SFDecodeNode* node);
};