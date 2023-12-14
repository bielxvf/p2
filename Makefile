SRC = src/main.c
BIN = p2
CFLAGS = -std=c18 -Wall -Wextra -Wpedantic -O3 -lsodium

build:
	mkdir build
	gcc $(SRC) $(CFLAGS) -o build/$(BIN)

clean:
	rm -rf build
