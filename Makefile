SRC = src/main.c
BIN = p2
CFLAGS = -Wall -Wextra -Wpedantic -O3 -lsodium -ltar

build:
	mkdir build
	@time --format="\n  Exit status: %x\n  Build time: %es" gcc $(SRC) $(CFLAGS) -o build/$(BIN)
	@echo "\nRun with \`./build/p2\`"

clean:
	@rm -rf build

rebuild: clean build
