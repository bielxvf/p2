SRC = src/main.c
BIN = p2
CFLAGS = -Wall -Wextra -Wpedantic

build:
	mkdir build
	gcc $(SRC) $(CFLAGS) -o build/$(BIN)

clean:
	rm -rf build/*
