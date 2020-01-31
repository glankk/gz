# Installing gz

**Note: The patcher tools create new files, they do not replace the files that
you provide.**

To use gz with an emulator or flash cart, follow the instructions for patching
a ROM file. In order to enable line features on N64, also follow the steps for
injecting the line microcode. The new ROM can be opened with an emulator or
transferred to a flash cart. For emulator usage, you will need to enable
Expansion Pak emulation. On some emulators, you may need to change CPU Core
Style to Interpreter. For use with a flash cart, your N64 will need an
Expansion Pak.

For use on the Wii, follow the instructions for patching a WAD file. The
patched WAD can be installed as a channel on an emulated Wii, or on a homebrew
Wii Console using a WAD manager. The original Ocarina of Time channel can be
installed alongside the patched WAD, and will not be overwritten. Homebrewing a
Wii console and using a WAD manager is not covered by this guide.

## Supported games
Ocarina of Time ROMs:

-   The Legend of Zelda: Ocarina of Time (USA) (1.0)
-   The Legend of Zelda: Ocarina of Time (USA) (1.1)
-   The Legend of Zelda: Ocarina of Time (USA) (1.2)
-   The Legend of Zelda: Ocarina of Time - Master Quest (USA) (GC)
-   The Legend of Zelda: Ocarina of Time (USA) (GC) (Master Quest Disc /
Collector's Edition)
-   Zeruda no Densetsu: Toki no Okarina (JPN) (1.0)
-   Zeruda no Densetsu: Toki no Okarina (JPN) (1.1)
-   Zeruda no Densetsu: Toki no Okarina (JPN) (1.2)
-   Zeruda no Densetsu: Toki no Okarina - GC Ura (JPN) (GC)
-   Zeruda no Densetsu: Toki no Okarina (JPN) (GC) (GC Ura Disc)
-   Zeruda no Densetsu: Toki no Okarina (JPN) (GC) (Zeruda Korekushon)

Ocarina of Time WADs:

-   The Legend of Zelda: Ocarina of Time (USA) (1.2) (Wii VC)
-   Zelda no Densetsu: Toki no Okarina (JPN) (1.2) (Wii VC)

Line microcode ROMs:

-   Hey You, Pikachu! (USA)
-   Hey You, Pikachu! (JPN)

## Using the patcher GUI
Start the gz-gui program. Click the tab that corresponds to the type of file
you want to create and fill out the options according to the instructions
below. Press Go! to run the patcher. The output log window will show
information about the patching process. When the patcher is done, you'll be
asked where you want to save the new file. Enter a filename and press Save.
When the output window closes, the new file has been saved and you can close
the program.

### Patching a ROM
In the ROM group box, press Select and open the ROM that you wish to patch. If
you want to enable line features, check Inject line microcode in the Line
microcode group box, then press Select and open one of the supported microcode
donor ROMs.

### Patching a WAD
Press the Select button in the WAD group box and open an Ocarina of Time WAD.
The ROM contained in the WAD will be used by default (USA 1.2 or JPN 1.2
depending on the WAD used). To use a different version, check Use external ROM
in the ROM group box, press Select and open the ROM that you wish to patch and
inject. The default options should be fine for most users.

## Using the command-line

### Patching a ROM
**Windows:** `patch-rom.bat [options...] <input-rom>`

**GNU/Linux:** `./patch-rom [options...] <input-rom>`

**macOS:** From `gz-gui.app/Contents/Resources`:
`./patch-rom [options...] <input-rom>`

`options`:

-   `-s`: Invoke as a sub-program to the gui patcher. Log messages are
    redirected to stderr, and the default filename is printed to stdout.
-   `-o <output-rom>`: Save as `<output-rom>` instead of the default filename.

### Patching a WAD
**Windows:** `patch-wad.bat [options...] <wad-file>`

**GNU/Linux:** `./patch-wad [options...] <wad-file>`

**macOS:** From `gz-gui.app/Contents/Resources`:
`./patch-wad [options...] <rom-file>`

`options`:

-   `-s`: Invoke as a sub-program to the gui patcher. Log messages are
    redirected to stderr, and the default filename is printed to stdout.
-   `-o <output-wad>`: Save as `<output-wad>` instead of the default filename.
-   `-m <input-rom>`: Patch and inject `<input-rom>` instead of using the ROM
    contained in the content file.
-   `-i|--channelid <id>`: Set the channel ID to `<id>` instead of the default.
-   `-t|--channeltitle <title>`: Set the channel title to `<title>` instead
    of the default.
-   `-k|--key <key-file>`: Use `<key-file>` as the common key instead of
    `common-key.bin`.
-   `-r|--region <region>`: Set the channel region to `<region>`. `<region>`
    should be one of the following:
    -   `0`: Japan
    -   `1`: USA
    -   `2`: Europe
    -   `3`: Region free (default)
-   `-d|--directory <content-directory>`: Use `<content-directory>` as working
    directory for the main content file. The default is `wadextract`.
-   `--raphnet`: Use Raphnet Adapter controller remapping. L is bound to the Z
    Trigger on the controller instead of Down on the C-stick.
-   `--disable-controller-remappings`: Disable all controller remappings.
-   `--no-homeboy`: Do not inject the homeboy binary and hooks. Homeboy
    features will be unavailable.

### Injecting the line microcode
**Windows:** `inject_ucode.bat <dst-file> <src-file>`

**GNU/Linux:** `./inject_ucode <dst-file> <src-file>`

**macOS:** `gz-gui.app/Contents/Resources/inject_ucode <dst-file> <src-file>`
