## User manual
- [Introduction](#introduction)
- [Warps menu](#warps-menu)
- [Scene menu](#scene-menu)
- [Cheats menu](#cheats-menu)
- [Inventory menu](#inventory-menu)
- [Equips menu](#equips-menu)
- [File menu](#file-menu)
- [Watches menu](#watches-menu)
- [Debug menu](#debug-menu)
- [Settings menu](#settings-menu)
- [Issues](#issues)

### Introduction
*Note: If you're using gz on the Wii VC, you should read the [Issues](#issues) section to find out about the differences in the VC version.*

The main interface for accessing the provided tools is the utility menu.
By default, this menu is brought up by pressing `R + L`,
but this button combination can be changed (see [Settings](#settings-menu)).
Use the D-Pad to navigate the menu, and L to make a selection.
For a description of each of the submenus, see their respective section below.

Beyond the tools provided by the utility menu there is also;
- **An input display.** The two numbers represent the x and y coordinate of the control stick.
The button icons that appear represent the buttons that are pressed on the controller.
*Enabled by default.*
- **A lag counter.** Displays lag by subtracting the number of game frames
passed from the game's vertical interrupt counter.
Displayed in units of frames (60Hz, default), or seconds. *Disabled by default.*
- **A timer.** Measures real-time using the CPU counter. *Disabled by default.*
- **_Various button-activated commands._**

These features can be configured from the settings menu (see [Settings](#settings-menu)).

### Warps menu
The **places** menu provides a list of all scenes and their respective entrances,
grouped into eight categories.
Selecting a scene with multiple entrances will show a list of all entrances for that scene.
Selecting an entrance will instantly warp you to that entrance.
Scenes with only one entrance will warp you to that entrance when selected,
without showing an entrance list.
If you want to warp to a specific entrance index,
you can enter that index in the warps menu and select **warp**.
The **age** and **cutscene** options specify which age Link will be at when performing a
warp, and which cutscene should be played for that scene.
These options apply to both the places menu and warping using an entrance index.

**clear cs pointer** will point the cutscene pointer to an empty cutscene,
which is useful for preventing certain wrong warps from crashing.
The bottom of the warps menu shows information about the game's current warp parameters.

**_Warning:_** Attempting to load a beta scene will cause a crash on all but the
debug version of Ocarina of Time.

**_Warning:_** Starting a game (with a warp or scene loading command) when no file data is
loaded (i.e. from the N64 logo and, to a lesser extent, the file select menu)
will cause *undefined behavior*.

### Scene menu
Selecting **explorer** will bring up the scene explorer,
which shows an overlay of the current scene.
Use the D-Pad up and down to navigate forwards and backwards through the scene, and
D-Pad left and right to rotate the view.
While holding Z, the D-Pad up and down will navigate vertically through the scene, and
D-Pad left and right will move sideways.
Use the `explore prev room` and `explore next room` commands
to cycle through the rooms of the scene (bound to `R + D-Down` and `R + D-Up` by default).
Pressing L will teleport link to the location and orientation of the crosshair and close the scene explorer.

**set entrance point** sets link's current position and orientation as the point where he will respawn after voiding out.
**clear flags** and **set flags** modifies the temporary and permanent flags for the current scene,
which keep track of things such as which chests have been opened,
which items have been collected, which enemies have been defeated, etc.
The **load room** option loads the room with the specified index,
if a room which such an index exists within the current scene.
If that room is already the currently loaded room, it will be unloaded.
**collision view**, when enabled, shows the static scene collision in the current scene.
The collision polygons are color-coded to show special properties;
- **Blue:** Hookshotable surface.
- **Purple:** Surface with special interaction (ladder, vine, crawlspace, not climbable, grabbable).
- **Red:** Void trigger.
- **Green:** Load trigger.
- **Light green:** Surface with special behavior (sand, quicksand, ice, lava, jabu wall, damaging wall, no recoil wall, void).
- **Light yellow:** Slippery slope.
- **White:** Normal surface.

Note that collision polygons are affected by scene lighting and fog, which can cause their appearance to be misleading.
The collision view **mode** decides how collision polygons are drawn.
The _decal_ setting will draw polygons overlaid on scene textures,
but will not produce any new surfaces (note however that most emulators do not correctly emulate this behavior).
The _surface_ setting draws collision polygons as their own surfaces,
but can produce depth flickering on existing scene textures.
Collision polygons can be configured to be completely opaque, or see-through with the **translucent** option.
The **reduced** option will reduce potential lag by only rendering collision polygons with some special property (i.e. colored polygons).
When entering a new scene, or changing a collision view setting, the collision view must be disabled and re-enabled in order to update.
**teleport slot** selects which position to save and load when using the
teleportation commands (this can also be bound to a button combination, but is unbound by default).
The bottom of the scene menu shows information about the current scene.

### Cheats menu
This menu allows toggling the builtin cheats on and off.
The following cheats are available:
- **energy:** Gives full health
- **magic:** Gives full magic
- **_various items:_** Sets item amounts to the current capacity of that item,
or to one if the capacity is zero or unlimited.
- **small keys:** Sets the number of small keys to one within the current dungeon, if any.
- **rupees:** Sets the rupee amount to the capacity of the current wallet.
- **nayru's love:** Prevents nayru's love from expiring, if active.
If nayru's love is not currently active,
entering a scene while this cheat is enabled will activate it.
- **freeze time:** Prevents the current time of day from advancing.
Does not affect sun's song or the time of day modifiers in the file menu.
- **no music:** Stops background music from playing.
- **items usable:** Clears all item restriction flags,
allowing all items to be used regardless of location.
- **no minimap:** Keeps the minimap hidden at all times.
- **isg:** Permanently activates the *infinite sword glitch*.

To undo the effects of the *no music*, *items usable*, and *no minimap* cheats,
turn the cheat off and enter a new scene, or reload the current scene.

### Inventory menu
The **equipment and items** menu lets you modify your equipment, C-button items,
and passive equipment items.
There is also an option to set whether or not the Giant's Knife has been broken,
and whether or not the Biggoron's Sword has been obtained.
Pressing a bottle, trade item, or equipment item will bring up an item wheel where you can
select from the possible items for that slot.
Use the D-Pad left and right to cycle through the items,
and the D-Pad up and down to cycle three items at a time.
Capacity equipment items (e.g. quiver, bomb bag etc.) that are not normally obtainable without
using cheats or glitches are denoted with an asterisk.

The **quest items** menu lets you modify all items on the Quest Status screen,
as well as your energy and magic.
There are also options to modify the dungeon items and small key amount of a specified dungeon.
**energy cap.**, **defense**, **magic cap.**, and **heart pieces** are specified in
hexadecimal for simplicity (from here on denoted with an *h*).
For energy capacity, 10h corresponds to one heart container.
The defense checkbox enables or disables double defense damage reduction,
and the field next to it modifies the number of defense heart containers.
For magic capacity, 0h is the normal capacity, 1h is double magic, 2h is what would be triple magic etc.
For heart pieces, 10h corresponds to one heart piece.

The **amounts** menu lets you modify the ammo of your C-button items,
your magic and energy amount, number of hits left on Giant's Knife, and rupee amount.
The number of Giant's Knife hits left is what decides whether or not the Giant's Knife / Biggoron's Sword
appears to be broken when Link wields it.
Magic, energy, and Giant's Knife hits are specified in hexadecimal.
30h magic is the max for normal magic capacity, and 60h is the max for double magic capacity.
For energy, 10h corresponds to one heart container.
Though the highest amount of rupees that can be specified is 99999,
entering a value greater than or equal to 65536 will wrap the amount around to zero and on.

### Equips menu
This menu lets you modify your current equips, and the items equipped to your B and C-buttons.
Pressing a piece of equipment that is already equipped will unequip it.
Unequpping a sword or equipping nothing to the B button will
automatically set the current file's swordless flag.
**_Warning:_** Unequipping boots will have strange effects, and usually cause an immediate crash.
*Note:* Changing a C-button equip will not modify the equipped item slot for that button.

### File menu
The **restore skulltulas** option clears all the gold skulltula flags in the current file,
thus restoring all destroyed gold skulltulas. The gold skulltulas in the current scene are not
affected until the scene is reloaded.
**call navi** sets Navi's advice timer to a value which will make her want to talk to you.
This will not have any effect until you enter an area where Navi normally appears.
The **memory file** selects the memory file to save to and load from when using the memory file commands
(see [Settings](#settings-menu)).
The current time of day can be manually adjusted,
or automatically fast-forwarded to day or night with the corresponding buttons.
**carpenters freed**, **intro cutscenes**, and **rewards obtained** let's you set and clear
various flags in the current file.
Pressing the checkmark will set the pertinent flags such that they have been "completed",
and the cross will clear them (setting them to the state that they were in when the file was created).

The **timer 1** and **timer 2** options display and modify the state of the two types of timers in the game.
The first timer is used for hot rooms and races, and the second timer is used for trade items
and the Castle Collapse sequence.
The numbers represent the number of seconds left on the timers,
and the options show the current state of the timers. Both of these can be modified manually.
*Note:* When the first timer is set to a heat state,
the game will instantly deactivate it if the current room is not "hot".
**_Warning:_** Modifying the state of the timers can yield strange behavior.
In general, it is safest to use the the *starting* and *stopped* states when doing this.

The **file index** option decides which file the game will be saved to when saving from
the start menu or the Game Over screen.
When the file index is set to FFh, as it is by default on the title screen,
saving will have no effect.
There's also a **language** and **z targeting** option, which apply to the current file.

Save files can be saved to and and loaded from an ED64 SD card with the **save to disk** and **load from disk** options.
Pressing _load from disk_ will bring up the file browser. Pressing the name of a save file will load it and return to the file menu.
The **load file to** and **after loading** options decide where in memory the file will be loaded to
(the current zelda file, the currently selected memfile slot, or both),
and what should happen when the loading is completed (reload scene, void out, or nothing).
The _after loading_ option will have no effect when loading only to the current memfile.
The file extension used for save files on disk is `.ootsave`, and the default filename is `file`.
The filename can be changed by pressing the name field. Select the position you want to change with the D-Pad left and D-Pad right,
and use the D-Pad up and D-Pad down to cycle between characters. Holding Z will cycle 3 characters at a time.
Pressing **clear** will set the name field to be empty. When the name field is empty, the default filename is `untitled`.
When saving, pressing the name of a save file in the file browser will copy that name to the name field.
Pressing accept will save the file to the current folder in the file browser with the specified file name.
If the file exists, you will be prompted to overwrite it.

### Watches menu
This menu lets you add custom RAM watches to observe arbitrary parts of game's memory in real-time.
Pressing the plus icon will add a new watch,
and pressing the cross next to a watch will remove that watch.
After adding a watch, enter a memory address and value type to display the value at that address.
These watch types are available:
- **u8:** one-byte value, unsigned.
- **s8:** one-byte value, signed.
- **x8:** one-byte value, hexadecimal.
- **u16:** two-byte value, unsigned.
- **s16:** two-byte value, signed.
- **x16:** two-byte value, hexadecimal.
- **u32:** four-byte value, unsigned.
- **s32:** four-byte value, signed.
- **x32:** four-byte value, hexadecimal.
- **f32:** four-byte value, IEEE 754 single-precision floating-point.

Pressing the anchor button next to a watch will release the watch from the watches menu
so that it's always visible, even when the menu is closed.
When a watch is released, a positioning button will appear which lets you change the position
of the watch on the screen.
Holding Z when positioning the watch will move it faster.

Watches can be imported from text files on an ED64 SD card by pressing the folder icon.
Press a watch file to bring up a list of all watches contained in that file. Press a watch to import it to your watch list.
When you've imported all the watches you need, press **return** to go back to the watches menu.
The format of watch files is described in the wiki, [here](https://github.com/glankk/gz/wiki/Watch-File-Syntax).

### Debug menu
_Note: These features are for advanced users. Be careful._

This menu contains various debug features to use for testing;
- **heap:** Displays information about the game's dynamic memory arena.
- **display lists:** Displays information about display list usage.
- **objects:** Shows a list of currently loaded objects, which contain graphical assets.
The **push** option loads the object file of the given id at the end of the list, and the **pop** option unloads the last object in the list.
- **actors:** Browse the currently loaded actors, and spawn new actors.
Select an actor type and use the arrows to scroll between all the loaded actors of that type.
The address and id of the selected actor is displayed below, as well as the actor variable in that actor instance.
The **delete** option deletes the currently selected actor, and the **go to** option teleports link to the location of that actor.
To spawn a new actor, enter an actor id, variable, the x, y, and z components of the position and rotation to spawn the actor at, and press **spawn**.
The **fetch from link** option loads link's current position and rotation into the position and rotation fields.
- **flags:** Display and edit saved game flags. The flags are grouped by the records they are kept in.
Use the arrows to cycle between flag records. Press a flag to toggle its state. A red flag is "off", and a green flag is "on".
The **log** menu displays a list of recent flag events. When a flag changes, its record, id, and new value is inserted at the top off the list.
The **undo** option reverts the effect of the most recent flag event and removes it from the log.
The **clear** option removes all flag events from the log, but does not affect the state of the given flags.
_Note:_ The flag log only records changes when the log menu is open. If a flag changes and then changes back while the log is closed,
these changes will not be recorded.
_Note:_ Scene-related flags only affect the currently loaded scene. If a scene flag event that occurred in one scene is undone in a different scene,
it will not have the desired effect.
- **memory:** Memory editor. Use the horizontal arrows to cycle between memory domains,
and the vertical arrows to scroll up and down between addresses. Holding Z while scrolling will scroll faster.
You can also enter an address manually in the address field. To edit memory, select the desired word size and press a memory cell to modify it.

### Settings menu
This is where most of the functionality of gz is configured.
The **profile** option selects which profile to save and load settings to and from.
When the game starts, the settings saved to profile zero are automatically loaded, if any.
The appearance of the menu can be configured with the **font** and **drop shadow** options.
Disabling drop shadow can reduce the graphical computation impact of the menu, but may also reduce readability.
The visibility of the on-screen display elements can be configured with the **input display**,
**lag counter**, **timer**, and **pause display** options.
The screen position of the utility menu, input display, lag counter, and timer can be configured
by their respective positioning buttons.
Holding Z when positioning an element will move it faster.
The display unit of the lag counter can be set to *frames* or *seconds*.
**macro input** enables or disables controller input when macro playback is active.
The **break type** option decides how the *break free* command will function.
When the break type is *normal*, the command will end textboxes, certain events and traditional cutscenes.
The *aggressive* break type will cause the command to also try to reset the camera and some of Link's state flags.
**save settings** and **load settings** will save and load the current settings to the
currently selected profile.
**restore defaults** will restore all saved settings to their default values (Does not affect saved profiles).
If the saved settings were to become corrupted in such a way that they prevent the game from starting,
holding the Start button when the game is starting will load the default settings instead of loading profile zero.
The following settings are saved:
- Menu and on-screen displays appearances and settings.
- Saved positions and the currently selected position slot number.
- Watches.
- Command button binds.
- Activated cheats.
- Warp menu age, cutscene index, and entrance index.
- Disk file loading settings.

The **commands** menu lets you bind commands to custom button combinations and/or activate them manually.
Pressing the name of a command will activate that command,
and pressing the button combo in the right column will bind a button combo to the corresponding command.
If you want to unbind a command, press and keep holding L when starting the binding.
A button combo for any given command can contain at most four buttons.
When activating a command with a button combo,
the button combo must explicitly be input the way it appears in the commands menu.
For example,
a command with the button combo `R + A` will only be activated if you press R first and then A,
or R and A at the same time.
`A + R`, or `R + B + A` will not activate the corresponding command.
If the set of buttons in one button combo is a subset of those in another button combo,
the former will be overridden by the latter when both are active simultaneously.

The following commands are available:
- **show/hide menu**: Opens the utility menu if it's closed, closes it if it's opened.
*Default: `R + L`*
- **return from menu:** Returns to the previous menu, as if the *return* button was pressed.
*Default: `R + D-Left`*
- **break free:** Attempts to break any effect that removes control of Link.
*Default: `C-Up + L`*
*VC Default: `Start + L`*
- **levitate:** The classic L to levitate command.
*Default: `L`*
- **save position:** Saves Link's current position and orientation to the current position slot.
*Default: `D-Left`*
- **load position:** Teleports Link to the position in the current position slot.
*Default: `D-Right`*
- **save memfile:** Saves the current state of the game to the memory file in the current memory file slot.
Everything that would be saved when saving the game normally is saved to the memory file.
*Default: `R + D-Left`*
- **load memfile:** Loads the state of the current memory file.
*Default: `R + D-Right`*
- **reset lag counter:** Resets the number of lag frames recorded to zero.
*Default: `R + B + D-Right`*
- **start/stop timer:** Starts the on-screen timer if it is stopped, stops it if it's running.
*Default: `R + A + D-Left`*
- **reset timer:** Sets the time of the on-screen timer to zero.
*Default: `R + B + D-Left`*
- **pause/unpause:** Pauses the gameplay, effectively freezing the state of the game.
If the game is already frozen, resumes gameplay as normal.
While the game is frozen, a pause icon will appear on the top-left of the screen
(enabled by default, can be turned off).
*Note:* Some elements, such as timers, textboxes, and certain particle effects operate independently of the game world,
and are thus unaffected by the *pause* and *frame advance* commands.
*Default: `D-Down`*
- **frame advance:** If the game is frozen by the pause command, advances one frame of gameplay.
Otherwise, freezes the game as if the pause command was activated.
*Default: `D-Up`*
- **file select:** Returns (or proceeds) to the game's file select menu.
*Default: `B + L`*
- **reload scene:** Reloads the current scene, starting from the last scene entrance.
*Default: `A + L`*
- **void out:** Reloads the current scene,
starting from the last room entrance as if Link voided out.
*Default: `A + B + L`*
- **reset:** Reset the game, as if the reset button had been pressed. All unsaved settings will be lost.
*Default: `unbound`*
- **turbo:** Sets Link's linear velocity to 27.
*Default: `unbound`*
- **fall:** Makes Link fall through the floor, as if there was no floor.
*Default: `unbound`*
- **toggle age:** Toggles between Adult and Child Link. Takes effect when entering a new area.
*Default: `unbound`*
- **start timer:** Starts the on-screen timer if it is stopped.
*Default: `unbound`*
- **stop timer:** Stops the on-screen timer if it is running.
*Default: `unbound`*
- **previous position:** Selects the previous position slot to be used for teleportation.
*Default: `unbound`*
- **next position:** Selects the next position slot to be used for teleportation.
*Default: `unbound`*
- **previous memfile:** Selects the previous memory file slot.
*Default: `unbound`*
- **next memfile:** Selects the next memory file slot.
*Default: `unbound`*
- **record macro:** Record/stop recording input macro. When starting macro recording, any previously recorded macro will be erased.
Input macros are not saved between resets.
*Default: `unbound`*
- **play macro:** Playback input macro. This command can be held down to loop macro playback.
By default, macro input will override controller input, but this can be changed with the **macro input** setting in the settings menu.
*Default: `unbound`*
- **explore prev room:** Loads the previous room while using the scene explorer.
*Default: `R + D-Down`*
- **explore next room:** Loads the next room while using the scene explorer.
*Default: `R + D-Up`*

**_Warning:_** Unbinding the *show/hide menu* or *return from menu* commands,
or binding them to a button combination that will interfere with menu navigation
can make it impossible to use the utility menu.
If this happens, you can restore the default settings by entering the following button sequence:
`D-Up D-Up D-Down D-Down D-Left D-Right D-Left D-Right B A`.

_Note:_ Button combos that interfere with menu navigation for commands that aren't related to
menuing are disabled while the utility menu is active.

### Issues
There are some known issues with the Wii VC version of gz;
- The D-Pad on the Classic/Gamecube Controller is mapped to the L button on the Virtual Console. The WAD patcher remaps the D-Pad to be functional again, and maps C-Stick Down to L on the Virtual Console to provide access to the utility menu.
- Disk features rely on the presence of the ED64 which does not exist on the Wii, and are therefore unavailable.
- The scene explorer has graphical glitches due to poor emulation.
- The reset command is not emulated correctly and has been removed.
- The timer function is not emulated correctly and has been removed.
