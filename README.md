# p2
Passman 2

# Dependencies
1. [libsodium](https://libsodium.org/)
1. [xclip](https://github.com/astrand/xclip)

## Arch
```sh
sudo pacman -S libsodium xclip
```

## Other distros
I run an Arch-based distribution so I have no idea

# Build
```sh
make clean
make build
```
The `p2` binary will be stored under `build/`

# Run
After building the binary, run it with `./build/p2`

# Install
Just copy the binary to somewhere in your `PATH`

# Usage
```sh
p2 -h
```

# Contribuiting
- Be as simple as possible
- Comment as little as possible. If you always feel like you need to explain what your code is doing, you're either wasting time or writing bad code.
