#include "archive_format.h"
#include <stdexcept>

void ArchiveWriter::writeHeader(std::ostream& out, const ArchiveHeader& header) {
    out.write(reinterpret_cast<const char*>(&header.signature), sizeof(header.signature));
    out.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));
    out.write(reinterpret_cast<const char*>(&header.algorithm), sizeof(header.algorithm));
    out.write(reinterpret_cast<const char*>(&header.frequencyBits), sizeof(header.frequencyBits));
    out.write(reinterpret_cast<const char*>(&header.reserved), sizeof(header.reserved));
    out.write(reinterpret_cast<const char*>(&header.originalSize), sizeof(header.originalSize));
    out.write(reinterpret_cast<const char*>(&header.compressedSize), sizeof(header.compressedSize));
}

void ArchiveWriter::writeFrequencies(std::ostream& out, const std::vector<uint64_t>& freqs, int bits) {
    if (bits == 64) {
        for (uint64_t freq : freqs) {
            out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
        }
    } else if (bits == 32) {
        for (uint64_t freq : freqs) {
            uint32_t freq32 = static_cast<uint32_t>(freq);
            out.write(reinterpret_cast<const char*>(&freq32), sizeof(freq32));
        }
    } else if (bits == 8) {
        for (uint64_t freq : freqs) {
            uint8_t freq8 = static_cast<uint8_t>(freq);
            out.write(reinterpret_cast<const char*>(&freq8), sizeof(freq8));
        }
    } else if (bits == 4) {
        for (size_t i = 0; i < freqs.size(); i += 2) {
            uint8_t byte = (freqs[i] & 0x0F) << 4;
            if (i + 1 < freqs.size()) {
                byte |= (freqs[i + 1] & 0x0F);
            }
            out.put(static_cast<char>(byte));
        }
    } else {
        throw std::invalid_argument("Unsupported bit size");
    }
}

ArchiveHeader ArchiveWriter::readHeader(std::istream& in) {
    ArchiveHeader header;
    in.read(reinterpret_cast<char*>(&header.signature), sizeof(header.signature));
    in.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
    in.read(reinterpret_cast<char*>(&header.algorithm), sizeof(header.algorithm));
    in.read(reinterpret_cast<char*>(&header.frequencyBits), sizeof(header.frequencyBits));
    in.read(reinterpret_cast<char*>(&header.reserved), sizeof(header.reserved));
    in.read(reinterpret_cast<char*>(&header.originalSize), sizeof(header.originalSize));
    in.read(reinterpret_cast<char*>(&header.compressedSize), sizeof(header.compressedSize));
    return header;
}

std::vector<uint64_t> ArchiveWriter::readFrequencies(std::istream& in, int bits) {
    std::vector<uint64_t> freqs(Common::ALPHABET_SIZE, 0);
    
    if (bits == 64) {
        for (size_t i = 0; i < freqs.size(); i++) {
            in.read(reinterpret_cast<char*>(&freqs[i]), sizeof(uint64_t));
        }
    } else if (bits == 32) {
        for (size_t i = 0; i < freqs.size(); i++) {
            uint32_t freq32;
            in.read(reinterpret_cast<char*>(&freq32), sizeof(uint32_t));
            freqs[i] = freq32;
        }
    } else if (bits == 8) {
        for (size_t i = 0; i < freqs.size(); i++) {
            uint8_t freq8 = in.get();
            freqs[i] = freq8;
        }
    } else if (bits == 4) {
        for (size_t i = 0; i < freqs.size(); i += 2) {
            uint8_t byte = in.get();
            freqs[i] = (byte >> 4) & 0x0F;
            if (i + 1 < freqs.size()) {
                freqs[i + 1] = byte & 0x0F;
            }
        }
    } else {
        throw std::invalid_argument("Unsupported bit size");
    }
    
    return freqs;
}