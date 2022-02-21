# Build

## Linux Dependencies

Some dependencies are handled as git submodules. The remaining dependencies are
(from the perspective of Arch `pacman`):

- `git`
- `gcc`
- `make`
- `cmake`
- `sdl2`
- `sdl2_image`
- `sdl2_ttf`
- `sdl2_mixer`

## Windows Dependencies

A build process involving MSYS2 on Windows 10 is supported.

Note that certain non-critical components of the submodule dependencies are
elided in this build process (eg, generating submodule docs).

1. Install [MSYS2](https://www.msys2.org/).
2. Start an MSYS2 MingW64 shell, ie, `C:\<msys64-install-root>\msys64\mingw64`.
3. Update packages with `pacman`:
    - `pacman -Syyu`
    - `pacman -Syu`
4. Install dependencies with `pacman`:
    - `pacman -S git`
    - `pacman -S mingw-w64-x86_64-make mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake`
    - `pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_gfx mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_ttf`
5. Establish an alias to `mingw32-make`, eg:
    - `echo 'alias make=/mingw64/bin/mingw32-make.exe' >> ~/.bashrc`
    - `source ~/.bashrc`

## Quickstart

Ensure above dependencies are installed for the target platform.

### Notes

`make init` pulls in all submodule dependencies, builds them as appropriate to
the platform, then builds the core project.

Thereafter, `make` is sufficient to rebuild after any changes are made to the
core project.

Note that the build process is designed to take advantage of the `-jN` option
for `make` (eg, `make -j8`), which can significantly improve build times
relative to the choice of `N` and the count of available CPU cores on the build
host.

### Linux Build

```
git clone <repo>
make init
```

### Windows Build

**Important**: `make` is aliased to `/mingw64/bin/mingw32-make.exe`.

```
git clone <repo>
make init-win
```

# Binaries

## Core binaries

After building, core binaries are available in `build`.

## Test binaries

After building, test binaries are available in `build_test`.
