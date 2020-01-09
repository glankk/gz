## Prerequisites
To build this software, you need to have the
[n64 tools](https://github.com/glankk/n64) installed. See the
[readme](https://github.com/glankk/n64/blob/master/README.md) in the n64
repository for instructions. To make a VC inject with a Wii WAD, you need
[gzinject](https://github.com/krimtonz/gzinject) to be installed.

## Building
To build all gz binaries, navigate to the root directory of the repository and
run `make`. To build binaries that will work correctly on the Wii VC, you must
have configured the n64 tools with `--enable-vc` when building the MIPS
toolchain.

## Patching
To create a UPS patch or a pre-patched rom, run `./make-patch <rom-file>`
or `./make-rom <rom-file>`. `<rom-file>` should be an unmodified rom to be
used for creating the patch. Use `./make-patch-vc <rom-file>` to create a
rom patch that targets the Wii VC. To create a patched Wii WAD with a gz rom
inject, run `./make-wad <wad-file>`.
