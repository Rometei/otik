#include "common.h"
#include "huffman.h"
#include "shannon_fano.h"
#include "archive_format.h"
#include "bitstream.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <stdexcept>

void decodeVersion1(std::istream& in, const std::string& outputFile) {
    std::cerr << "Version 1 format not supported in this implementation" << std::endl;
    throw std::runtime_error("Unsupported version");
}

void decodeVersion2Huffman(std::istream& in, const ArchiveHeader& header, const std::string& outputFile) {
    auto freqs = ArchiveWriter::readFrequencies(in, header.frequencyBits);
    HuffmanDecoder decoder(freqs);
    
    std::stringstream compressedStream;
    char buffer[4096];
    uint64_t remaining = header.compressedSize;
    
    while (remaining > 0 && in.read(buffer, std::min(sizeof(buffer), static_cast<size_t>(remaining)))) {
        size_t read = in.gcount();
        compressedStream.write(buffer, read);
        remaining -= read;
    }
    
    compressedStream.seekg(0);
    BitInputStream bitIn(compressedStream);
    
    auto decodedData = decoder.decodeData(bitIn, header.originalSize);
    
    std::ofstream output(outputFile, std::ios::binary);
    if (!output) {
        std::cerr << "Cannot create output file: " << outputFile << std::endl;
        return;
    }
    
    output.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    output.close();
    
    std::cout << "Huffman decompression completed: " << decodedData.size() << " bytes written" << std::endl;
}

void decodeVersion3ShannonFano(std::istream& in, const ArchiveHeader& header, const std::string& outputFile) {
    auto freqs = ArchiveWriter::readFrequencies(in, header.frequencyBits);
    ShannonFanoDecoder decoder(freqs);
    
    std::stringstream compressedStream;
    char buffer[4096];
    uint64_t remaining = header.compressedSize;
    
    while (remaining > 0 && in.read(buffer, std::min(sizeof(buffer), static_cast<size_t>(remaining)))) {
        size_t read = in.gcount();
        compressedStream.write(buffer, read);
        remaining -= read;
    }
    
    compressedStream.seekg(0);
    BitInputStream bitIn(compressedStream);
    
    auto decodedData = decoder.decodeData(bitIn, header.originalSize);
    
    std::ofstream output(outputFile, std::ios::binary);
    if (!output) {
        std::cerr << "Cannot create output file: " << outputFile << std::endl;
        return;
    }
    
    output.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
    output.close();
    
    std::cout << "Shannon-Fano decompression completed: " << decodedData.size() << " bytes written" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input archive> <output file>" << std::endl;
        return 1;
    }
    
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Cannot open input archive: " << argv[1] << std::endl;
        return 1;
    }
    
    try {
        ArchiveHeader header = ArchiveWriter::readHeader(input);
        
        if (header.signature != Common::SIGNATURE) {
            std::cerr << "Invalid signature: expected HUFF, got different" << std::endl;
            return 1;
        }
        
        switch (header.version) {
            case Common::VERSION_1:
                decodeVersion1(input, argv[2]);
                break;
            case Common::VERSION_2:
                if (header.algorithm == Common::ALGO_HUFFMAN) {
                    decodeVersion2Huffman(input, header, argv[2]);
                } else {
                    std::cerr << "Unsupported algorithm for version 2: " << static_cast<int>(header.algorithm) << std::endl;
                    return 1;
                }
                break;
            case Common::VERSION_3:
                if (header.algorithm == Common::ALGO_SHANNON_FANO) {
                    decodeVersion3ShannonFano(input, header, argv[2]);
                } else {
                    std::cerr << "Unsupported algorithm for version 3: " << static_cast<int>(header.algorithm) << std::endl;
                    return 1;
                }
                break;
            default:
                std::cerr << "Unsupported version: " << static_cast<int>(header.version) << std::endl;
                return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return 1;
    }
    
    input.close();
    return 0;
}