#include "huffman.h"
#include <iostream>

HuffmanEncoder::HuffmanEncoder(const std::vector<uint64_t>& frequencies) : root(nullptr) {
    buildTree(frequencies);
}

HuffmanEncoder::~HuffmanEncoder() {
    if (root) clearTree(root);
}

void HuffmanEncoder::buildTree(const std::vector<uint64_t>& frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
    
    // Проверяем, есть ли ненулевые частоты
    bool hasNonZero = false;
    for (size_t i = 0; i < frequencies.size(); i++) {
        if (frequencies[i] > 0) {
            pq.push(new HuffmanNode(static_cast<uint8_t>(i), frequencies[i]));
            hasNonZero = true;
        }
    }
    
    if (!hasNonZero) {
        // Если все частоты нулевые, создаем фиктивный узел для одного символа
        pq.push(new HuffmanNode(0, 1));
    }
    
    if (pq.empty()) {
        throw std::runtime_error("Cannot build Huffman tree: no symbols with non-zero frequency");
    }
    
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }
    
    root = pq.top();
    
    std::vector<bool> code;
    generateCodes(root, code);
    
    // Проверяем, что сгенерировались коды
    if (codes.empty()) {
        throw std::runtime_error("No Huffman codes generated");
    }
}

void HuffmanEncoder::generateCodes(HuffmanNode* node, std::vector<bool>& code) {
    if (!node) return;
    
    if (!node->left && !node->right) {
        codes[node->symbol] = code;
        return;
    }
    
    if (node->left) {
        code.push_back(false);
        generateCodes(node->left, code);
        code.pop_back();
    }
    
    if (node->right) {
        code.push_back(true);
        generateCodes(node->right, code);
        code.pop_back();
    }
}

void HuffmanEncoder::encodeData(const std::vector<uint8_t>& data, BitOutputStream& out) const {
    if (codes.empty()) {
        throw std::runtime_error("No Huffman codes available for encoding");
    }
    
    for (uint8_t byte : data) {
        const auto& code = codes.at(byte);
        for (bool bit : code) {
            out.writeBit(bit);
        }
    }
}

void HuffmanEncoder::clearTree(HuffmanNode* node) {
    if (!node) return;
    if (node->left) clearTree(node->left);
    if (node->right) clearTree(node->right);
    delete node;
}

HuffmanDecoder::HuffmanDecoder(const std::vector<uint64_t>& frequencies) : root(nullptr) {
    buildTree(frequencies);
}

HuffmanDecoder::~HuffmanDecoder() {
    if (root) clearTree(root);
}

void HuffmanDecoder::buildTree(const std::vector<uint64_t>& frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
    
    bool hasNonZero = false;
    for (size_t i = 0; i < frequencies.size(); i++) {
        if (frequencies[i] > 0) {
            pq.push(new HuffmanNode(static_cast<uint8_t>(i), frequencies[i]));
            hasNonZero = true;
        }
    }
    
    if (!hasNonZero) {
        pq.push(new HuffmanNode(0, 1));
    }
    
    if (pq.empty()) {
        throw std::runtime_error("Cannot build Huffman tree for decoding");
    }
    
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }
    
    root = pq.top();
}

std::vector<uint8_t> HuffmanDecoder::decodeData(BitInputStream& in, size_t originalSize) const {
    if (!root) {
        throw std::runtime_error("Huffman tree not initialized for decoding");
    }
    
    std::vector<uint8_t> result;
    result.reserve(originalSize);
    
    HuffmanNode* current = root;
    
    for (size_t i = 0; i < originalSize; i++) {
        current = root;
        
        // Декодируем один символ
        while (current->left || current->right) {
            if (in.eof()) {
                throw std::runtime_error("Unexpected end of stream during decoding");
            }
            
            bool bit = in.readBit();
            current = bit ? current->right : current->left;
            
            if (!current) {
                throw std::runtime_error("Invalid Huffman code encountered");
            }
        }
        
        result.push_back(current->symbol);
    }
    
    return result;
}

void HuffmanDecoder::clearTree(HuffmanNode* node) {
    if (!node) return;
    if (node->left) clearTree(node->left);
    if (node->right) clearTree(node->right);
    delete node;
}