## Prerequisites
To build this software, you need to have the [n64 toolchain](https://github.com/glankk/n64) installed.
See the readme in the n64 repository for instructions.

## Environment
Before the project can be built, some environment variables will need to be configured.
To build from Code::Blocks, this can be done by launching Code::Blocks from a previously configured terminal,
by configuring the system-wide environment variables, or by using the EnvVar plugin.
To build from a terminal, they can be configured manually, or added to the system-wide environment variables.
These environment variables need to be configured before building;
- `EMUDIR` should point to the root directory of your BizHawk installation if you want to use it with Code::Blocks.
- `N64ROOT` must point to the root directory of the n64 repository.
- `PATH` needs to include the directory of the GNU Toolchain executables and GNU make (typically
  `C:\msys64\opt\n64-toolchain-slim\bin` and `C:\msys64\usr\bin`), and the `bin` subfolder of the n64 repository.

## Building
To build all gz binaries from a terminal, navigate to the root directory of the repository and run `make`.

To build and run the project with Code::Blocks and BizHawk, open `gz.cbp` with Code::Blocks, and navigate to Project -> Build options.
In the left pane, select the target you wish to configure. In the Custom variables tab, configure the following variables;
- `emurom` should point to the rom you wish to load when launching BizHawk.
- `emuarg` can optionally contain additional parameters to pass on to the emulator. For example, to automatically
  load a savestate after loading the rom: `--load-state=path/to/save.State`.

These paths should be relative to the root directory of your BizHawk installation.
Select Build -> Build and Run (or press F9). This will build the target and launch BizHawk.
Once the rom is running, open the Lua Console (Tools -> Lua Console). Click Open Script and select `patch.lua` from
the Lua subfolder of your BizHawk installation. This will load the patch into the game's memory.

## Patching
To create a UPS patch or a pre-patched rom, open a terminal in the `patch` directory and run `make-patch <rom-file>` or `make-rom <rom-file>`.
`<rom-file>` should be an unmodified rom to be used for creating the patch. You can also drag-and-drop the rom onto the script file.
