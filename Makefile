SRC = src/main.c
BIN = p2
CFLAGS = -std=c18 -Wall -Wextra -Wpedantic -O3 -lsodium

build:
	mkdir build
	@time --format="\n  Exit status: %x\n  Build time: %es" gcc $(SRC) $(CFLAGS) -o build/$(BIN)
	@echo -e "\nRun with \`./build/p2\`"

clean:
	@rm -rf build

rebuild: clean build
