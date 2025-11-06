CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

SOURCES = huffman.cpp frequency.cpp archive_format.cpp shannon_fano.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: encoder encoder_sf decoder analyzer comparison

encoder: encoder.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o encoder encoder.cpp $(OBJECTS)

encoder_sf: encoder_sf.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o encoder_sf encoder_sf.cpp $(OBJECTS)

decoder: decoder.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o decoder decoder.cpp $(OBJECTS)

analyzer: analyzer.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o analyzer analyzer.cpp $(OBJECTS)

comparison: comparison.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o comparison comparison.cpp $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f encoder encoder_sf decoder analyzer comparison $(OBJECTS)

.PHONY: all clean