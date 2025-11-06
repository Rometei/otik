#include "common.h"
#include "shannon_fano.h"
#include "archive_format.h"
#include "bitstream.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <stdexcept>

void decodeShannonFano(std::istream& in, const ArchiveHeader& header, const std::string& outputFile) {
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
            std::cerr << "Invalid signature" << std::endl;
            return 1;
        }
        
        if (header.version == Common::VERSION_3 && header.algorithm == Common::ALGO_SHANNON_FANO) {
            decodeShannonFano(input, header, argv[2]);
        } else {
            std::cerr << "Unsupported version or algorithm: version=" << static_cast<int>(header.version) 
                      << ", algorithm=" << static_cast<int>(header.algorithm) << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return 1;
    }
    
    input.close();
    return 0;
}