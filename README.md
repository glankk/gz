## About

This is a trainer for The Legend of Zelda: Ocarina of Time. It is compatible with version 1.0 and 1.2 of the NTSC release.

## Dependencies
* The n64-toolchain is required for building it can be found [here](https://github.com/glankk/n64).
* In installing the n64-toolchain, Code::Blocks IDE should be configured to support targeting the n64. Alonside Code::Blocks, BizHawk should also be installed and configured, including all environment variables.

## Building
***Note**: When switching build targets/versions, the project **must be cleaned**. Failing to do so will result in incompatible binaries being built which can and likely will cause the game to halt or lock.*
* From within Code::Blocks, select the target version (1.0/1.2) and build or run.
* Navigate to the bin/ directory where the project is stored, execute the following command: 
    * `luapatch.exe patch-data.lua -bin 0x80600000  gz.elf.bin`
* The newly generated `patch-data.lua` file should then be placed in the same directory as the `patch.lua` file that was handled in the n64-toolchain configuration.
* Load the ROM in BizHawk if it was not automatically loaded via Code::Blocks using the run command. Import the GameShark codes into BizHawk via the `Tools > GameShark Converter` menu item, or enable them if they have been imported and saved previously. Paste the code from the `.gsc` file relative to the ROM version being targetted.
* Open the Lua Console located under `Tools > Lua Console`. From here select `Script > Open Script...` and navigate to the `patch.lua` script (**not** patch-data.lua).

## Running

In order to build and run automatically, configure directories as follows:

**Rom Locations** (Paths are relative to the Code::Blocks project (.cbp) file)
* **1.0**
	* `..\n64\roms\Zelda no Densetsu - Toki no Ocarina v1.0 (Japan).z64`
* **1.2**
	* `..\n64\roms\Zelda no Densetsu - Toki no Ocarina (Japan) (Rev B).z64`