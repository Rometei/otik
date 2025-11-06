// archive_format.h - формат архива
#pragma once
#include "common.h"
#include <vector>
#include <iostream>

struct ArchiveHeader {
    uint32_t signature;
    uint8_t version;
    uint8_t algorithm;
    uint8_t frequencyBits;
    uint8_t reserved;
    uint64_t originalSize;
    uint64_t compressedSize;
    
    static const size_t SIZE = 24;
};

class ArchiveWriter {
public:
    static void writeHeader(std::ostream& out, const ArchiveHeader& header);
    static void writeFrequencies(std::ostream& out, const std::vector<uint64_t>& freqs, int bits);
    static ArchiveHeader readHeader(std::istream& in);
    static std::vector<uint64_t> readFrequencies(std::istream& in, int bits);
};