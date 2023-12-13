# p2
Passman 2

# Dependencies
p2 depends on [monocypher](https://monocypher.org/) to encrypt passwords

## Arch
```sh
git clone sudo pacman https://aur.archlinux.org/monocypher.git
cd monocypher
makepkg -si
```

## Other distros
I run an Arch-based distribution so I have no idea if others have a package for `monocypher` or not,
 in any case, if you cannot find a way to install it on your distro, download the official tarball [here](https://monocypher.org/download/)
 and follow the instructions provided on the `Getting started` section on their [home page](https://monocypher.org/)

# Build
```sh
make clean
make build
```
The `p2` binary will be stored under `build/`

# Run
After building the binary, run it with `./build/p2`

# Install
After building the binary, copy it somewhere in your path, such as `~/.local/bin/`, or for a system-wide usability, copy it to `/usr/bin/` or `/usr/local/bin/`

# Usage
```sh
p2 -h
```

# TODO
- [x] List subcommand
- [ ] New subcommand
- [ ] Print subcommand
- [ ] Remove subcommand
- [ ] Copy subcommand
- [ ] Backup subcommand
- [ ] Restore subcommand