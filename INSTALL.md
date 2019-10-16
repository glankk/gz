## Installing gz
The currently supported games are:

-   The Legend of Zelda: Ocarina of Time (NTSC-U/J 1.0)
-   The Legend of Zelda: Ocarina of Time (NTSC-U/J 1.1)
-   The Legend of Zelda: Ocarina of Time (NTSC-U/J 1.2)
-   The Legend of Zelda: Ocarina of Time (NTSC-U/J 1.2 - Wii VC)
-   The Legend of Zelda: Ocarina of Time - Master Quest (NTSC-J)
-   The Legend of Zelda: Ocarina of Time - Master Quest (NTSC-J - Wii VC)

### Using gz with an emulator / flash cart
**Windows:** Drag and drop a compatible rom onto the `patch.bat` script.

**GNU/Linux:** Run `./patch <rom-file>`.

A patched rom will be created. It can be played with an emulator or transferred
to a flash cart. For emulator usage, you will need to enable Expansion Pak
emulation. On some emulators, you may need to change CPU Core Style to
Interpreter. For use with a flash cart, your N64 will need an Expansion Pak.

### Using gz on the Wii VC
**Windows:** Drag and drop a compatible Wii WAD onto the `patch-wad.bat`
script.

**GNU/Linux:** Run `./patch-wad <wad-file>`.

A patched WAD file will be created.
This WAD can be installed as a channel on an emulated Wii, or on a homebrew Wii
Console using a WAD manager. The original Ocarina of Time channel can be
installed alongside the patched WAD, and will not be overwritten. Homebrewing a
Wii console and using a WAD manager is not covered by this guide.

### Enabling line features on N64
**Windows:** Select both your patched rom and a microcode rom, then drag and
drop them onto the `inject_ucode.bat` script.

**GNU/Linux:** Run `./inject_ucode <patched-rom> <ucode-rom>`.

The currently known and supported microcode roms are `Hey You, Pikachu! (U)`
and `Hey You, Pikachu! (J)`.
