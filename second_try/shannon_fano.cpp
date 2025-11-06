#include "shannon_fano.h"
#include <iostream>
#include <algorithm>
#include <queue>

ShannonFanoEncoder::ShannonFanoEncoder(const std::vector<uint64_t>& frequencies) {
    // Создаем узлы для символов с ненулевой частотой
    std::vector<SFNode> nodes;
    for (size_t i = 0; i < frequencies.size(); i++) {
        if (frequencies[i] > 0) {
            nodes.emplace_back(static_cast<uint8_t>(i), frequencies[i]);
        }
    }
    
    if (nodes.empty()) {
        // Если все частоты нулевые, добавляем фиктивный узел
        nodes.emplace_back(0, 1);
    }
    
    // Сортируем узлы по убыванию частот
    std::sort(nodes.begin(), nodes.end(), 
              [](const SFNode& a, const SFNode& b) { 
                  return a.frequency > b.frequency; 
              });
    
    // Вычисляем общую частоту
    uint64_t totalFreq = 0;
    for (const auto& node : nodes) {
        totalFreq += node.frequency;
    }
    
    // Строим коды рекурсивно
    currentCode.clear();
    buildCodes(nodes, 0, nodes.size() - 1, totalFreq);
    
    // Заполняем таблицу кодов
    for (const auto& node : nodes) {
        codes[node.symbol] = node.code;
    }
}

void ShannonFanoEncoder::buildCodes(const std::vector<SFNode>& nodes, int start, int end, uint64_t totalFreq) {
    if (start > end) return;
    
    if (start == end) {
        // Листовой узел - присваиваем текущий код
        const_cast<SFNode&>(nodes[start]).code = currentCode;
        return;
    }
    
    // Ищем оптимальную точку разделения
    uint64_t leftSum = 0;
    int splitIndex = start;
    uint64_t minDiff = UINT64_MAX;
    
    for (int i = start; i <= end; i++) {
        leftSum += nodes[i].frequency;
        uint64_t rightSum = totalFreq - leftSum;
        uint64_t diff = (leftSum > rightSum) ? (leftSum - rightSum) : (rightSum - leftSum);
        
        if (diff <= minDiff) {
            minDiff = diff;
            splitIndex = i;
        } else {
            break;
        }
    }
    
    // Добавляем 0 для левой части
    currentCode.push_back(false);
    buildCodes(nodes, start, splitIndex, leftSum);
    currentCode.pop_back();
    
    // Добавляем 1 для правой части
    currentCode.push_back(true);
    buildCodes(nodes, splitIndex + 1, end, totalFreq - leftSum);
    currentCode.pop_back();
}

void ShannonFanoEncoder::encodeData(const std::vector<uint8_t>& data, BitOutputStream& out) const {
    for (uint8_t byte : data) {
        const auto& code = codes.at(byte);
        for (bool bit : code) {
            out.writeBit(bit);
        }
    }
}

ShannonFanoDecoder::ShannonFanoDecoder(const std::vector<uint64_t>& frequencies) : root(nullptr) {
    buildTree(frequencies);
}

ShannonFanoDecoder::~ShannonFanoDecoder() {
    if (root) clearTree(root);
}

void ShannonFanoDecoder::buildTree(const std::vector<uint64_t>& frequencies) {
    // Создаем узлы для символов с ненулевой частотой
    std::vector<SFNode> nodes;
    for (size_t i = 0; i < frequencies.size(); i++) {
        if (frequencies[i] > 0) {
            nodes.emplace_back(static_cast<uint8_t>(i), frequencies[i]);
        }
    }
    
    if (nodes.empty()) {
        nodes.emplace_back(0, 1);
    }
    
    // Сортируем узлы по убыванию частот
    std::sort(nodes.begin(), nodes.end(), 
              [](const SFNode& a, const SFNode& b) { 
                  return a.frequency > b.frequency; 
              });
    
    // Строим дерево декодирования
    root = new SFDecodeNode();
    std::function<void(SFDecodeNode*, const std::vector<SFNode>&, int, int, uint64_t)> buildNode;
    
    buildNode = [&](SFDecodeNode* node, const std::vector<SFNode>& nodes, int start, int end, uint64_t totalFreq) {
        if (start > end) return;
        
        if (start == end) {
            // Листовой узел
            node->symbol = nodes[start].symbol;
            return;
        }
        
        // Находим точку разделения (аналогично кодировщику)
        uint64_t leftSum = 0;
        int splitIndex = start;
        uint64_t minDiff = UINT64_MAX;
        
        for (int i = start; i <= end; i++) {
            leftSum += nodes[i].frequency;
            uint64_t rightSum = totalFreq - leftSum;
            uint64_t diff = (leftSum > rightSum) ? (leftSum - rightSum) : (rightSum - leftSum);
            
            if (diff <= minDiff) {
                minDiff = diff;
                splitIndex = i;
            } else {
                break;
            }
        }
        
        // Создаем дочерние узлы
        node->left = new SFDecodeNode();
        node->right = new SFDecodeNode();
        
        buildNode(node->left, nodes, start, splitIndex, leftSum);
        buildNode(node->right, nodes, splitIndex + 1, end, totalFreq - leftSum);
    };
    
    uint64_t totalFreq = 0;
    for (const auto& node : nodes) {
        totalFreq += node.frequency;
    }
    
    buildNode(root, nodes, 0, nodes.size() - 1, totalFreq);
}

std::vector<uint8_t> ShannonFanoDecoder::decodeData(BitInputStream& in, size_t originalSize) const {
    std::vector<uint8_t> result;
    result.reserve(originalSize);
    
    for (size_t i = 0; i < originalSize; i++) {
        result.push_back(decodeSymbol(in));
    }
    
    return result;
}

uint8_t ShannonFanoDecoder::decodeSymbol(BitInputStream& in) const {
    SFDecodeNode* current = root;
    
    while (current->left || current->right) {
        bool bit = in.readBit();
        current = bit ? current->right : current->left;
        
        if (!current) {
            throw std::runtime_error("Invalid Shannon-Fano code encountered");
        }
    }
    
    return current->symbol;
}

void ShannonFanoDecoder::clearTree(SFDecodeNode* node) {
    if (!node) return;
    if (node->left) clearTree(node->left);
    if (node->right) clearTree(node->right);
    delete node;
}