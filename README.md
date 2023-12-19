# p2
Passman 2

# Dependencies
[libsodium](https://libsodium.org/)

## Arch
```sh
sudo pacman -S libsodium
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

# TODO
- [x] List subcommand
- [x] New subcommand
- [x] Print subcommand
- [x] Remove subcommand
- [ ] Copy subcommand
- [ ] Backup subcommand
- [ ] Restore subcommand