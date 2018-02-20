## Prerequisites
To build this software, you need to have the [n64 tools](https://github.com/glankk/n64) installed.
See the [readme](https://github.com/glankk/n64/blob/master/README.md) in the n64 repository for instructions.
To make a VC inject with a Wii WAD, you need [gzinject](https://github.com/krimtonz/gzinject) to be installed.

## Building
To build all gz binaries, navigate to the root directory of the repository and run `make`.
To build binaries that will work correctly on the Wii VC,
you must have configured the n64 tools with `--enable-vc` when building the MIPS toolchain.

## Patching
To create a UPS patch or a pre-patched rom, run `patch/make-patch <rom-file>` or `patch/make-rom <rom-file>`.
`<rom-file>` should be an unmodified rom to be used for creating the patch.
Use `patch/make-patch-vc <rom-file>` to create a rom patch that targets the Wii VC.
To create a patched Wii WAD with a gz rom inject, run `patch/make-wad <wad-file>`.
On windows, you can drag-and-drop the rom onto the script files in the `patch` directory.

## Building and running with Code::Blocks and BizHawk
You should first have configured the `EMUDIR` environment variable as described in the [n64 readme](https://github.com/glankk/n64/blob/master/README.md).
Open `gz.cbp` with Code::Blocks, and navigate to Project -> Build options.
In the left pane, select the target you wish to configure. In the Custom variables tab, configure the following variables;
- `emurom` should point to the (Ocarina of Time) rom you wish to load when launching BizHawk.
- `emuarg` can optionally contain additional parameters to pass on to the emulator. For example, to automatically
  load a savestate after loading the rom: `--load-state=path/to/save.State`.

These paths should be relative to the root directory of your BizHawk installation.
Select Build -> Build and Run (or press F9). This will build the target and launch BizHawk.
Once the rom is running, open the Lua Console (Tools -> Lua Console). Click Open Script and select `patch.lua` from
the Lua subfolder of your BizHawk installation. This will load the patch into the game's memory.
