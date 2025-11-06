// bitstream.h - побитовые потоки
#pragma once
#include <fstream>
#include <vector>

class BitOutputStream {
public:
    explicit BitOutputStream(std::ostream& os) : out(os), buffer(0), bitCount(0) {}
    
    void writeBit(bool bit) {
        buffer = (buffer << 1) | (bit ? 1 : 0);
        bitCount++;
        if (bitCount == 8) {
            out.put(static_cast<char>(buffer));
            buffer = 0;
            bitCount = 0;
        }
    }
    
    void writeBits(uint32_t bits, int count) {
        for (int i = count - 1; i >= 0; i--) {
            writeBit((bits >> i) & 1);
        }
    }
    
    void flush() {
        if (bitCount > 0) {
            buffer <<= (8 - bitCount);
            out.put(static_cast<char>(buffer));
            buffer = 0;
            bitCount = 0;
        }
    }
    
private:
    std::ostream& out;
    uint8_t buffer;
    int bitCount;
};

class BitInputStream {
public:
    explicit BitInputStream(std::istream& is) : in(is), buffer(0), bitCount(0) {}
    
    bool readBit() {
        if (bitCount == 0) {
            buffer = in.get();
            bitCount = 8;
        }
        return (buffer >> (--bitCount)) & 1;
    }
    
    uint32_t readBits(int count) {
        uint32_t result = 0;
        for (int i = 0; i < count; i++) {
            result = (result << 1) | (readBit() ? 1 : 0);
        }
        return result;
    }
    
    bool eof() const { return in.eof(); }
    
private:
    std::istream& in;
    uint8_t buffer;
    int bitCount;
};