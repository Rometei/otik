CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = huffman
SOURCES = main.c huffman.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) -lm

clean:
	rm -f $(TARGET)

.PHONY: clean