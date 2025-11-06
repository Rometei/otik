#pragma once
#include "common.h"
#include "bitstream.h"
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <iostream>

struct HuffmanNode {
    uint8_t symbol;
    uint64_t frequency;
    HuffmanNode* left;
    HuffmanNode* right;
    uint8_t minSymbol; // для детерминированного порядка
    
    HuffmanNode(uint8_t s, uint64_t f) 
        : symbol(s), frequency(f), left(nullptr), right(nullptr), minSymbol(s) {}
        
    HuffmanNode(HuffmanNode* l, HuffmanNode* r)
        : symbol(0), frequency(l->frequency + r->frequency), 
          left(l), right(r), minSymbol(std::min(l->minSymbol, r->minSymbol)) {}
};

struct CompareNode {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (a->frequency != b->frequency)
            return a->frequency > b->frequency;
        return a->minSymbol > b->minSymbol;
    }
};

class HuffmanEncoder {
public:
    HuffmanEncoder(const std::vector<uint64_t>& frequencies);
    ~HuffmanEncoder();
    
    std::map<uint8_t, std::vector<bool>> getCodes() const { return codes; }
    void encodeData(const std::vector<uint8_t>& data, BitOutputStream& out) const;
    
private:
    void buildTree(const std::vector<uint64_t>& frequencies);
    void generateCodes(HuffmanNode* node, std::vector<bool>& code);
    void clearTree(HuffmanNode* node);
    
    HuffmanNode* root;
    std::map<uint8_t, std::vector<bool>> codes;
};

class HuffmanDecoder {
public:
    HuffmanDecoder(const std::vector<uint64_t>& frequencies);
    ~HuffmanDecoder();
    
    std::vector<uint8_t> decodeData(BitInputStream& in, size_t originalSize) const;
    
private:
    void buildTree(const std::vector<uint64_t>& frequencies);
    void clearTree(HuffmanNode* node);
    
    HuffmanNode* root;
};