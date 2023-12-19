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
- [ ] Make it pretty

# Contribuiting
- Be as simple as possible
- Be as explicit as possible
- Name variables with descriptive names and with [snake_casing](https://en.wikipedia.org/wiki/Snake_case)
- Name functions with descriptive names and with [PascalCasing/UpperCamelCasing](https://en.wikipedia.org/wiki/Camel_case)
- Add your name to the 'Author(s)' comment on top of files (if you care)
- Commit a lot and commit often, 10 small commits wiht 1 conflict is better than one large commit that can only be merged manually because of conflicts
- Comment as little as possible, and if you do, your comments should explain *why* not *what* the code is doing. If you feel like you need to explain what your code is doing in a comment, I'm sorry to tell you, but it's bad code.
