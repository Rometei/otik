CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

# Универсальный декодер
UNIVERSAL_TARGET = universal_decoder
UNIVERSAL_SOURCES = main_universal.c universal_decoder.c


all: $(UNIVERSAL_TARGET) $(OLD_TARGET)

$(UNIVERSAL_TARGET): $(UNIVERSAL_SOURCES)
	$(CC) $(CFLAGS) -o $(UNIVERSAL_TARGET) $(UNIVERSAL_SOURCES)


clean:
	rm -f $(UNIVERSAL_TARGET)

test: all
	@echo "Testing universal decoder..."
	./$(UNIVERSAL_TARGET) -l
	./$(UNIVERSAL_TARGET) -v

.PHONY: all clean test