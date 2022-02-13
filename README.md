Build
=====

Dependencies
------------

Some dependencies are handled as git submodules. The remaining dependencies are
(from the perspective of Arch `pacman`):

- `sdl2`
- `sdl2_image`
- `sdl2_ttf`
- `sdl2_mixer`

Quickstart
----------

Ensure above dependencies are installed. Then:

```
git clone <repo>
make init
```

`make init` pulls in all submodule dependencies, builds them as relevant, then
builds the make project.

Thereafter, `make` is sufficient to build after any changes are made to the core project.

Adding the `-jN` flag to `make`, a la `make -j4`, can speed up build times.

Binaries
========

Core binaries
-------------

After building, core binaries are available in `build`.

Test binaries
-------------

After building, test binaries are available in `build_test`.
