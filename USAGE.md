# User manual

## 0 Table of contents
-   [0 Table of contents](#0-table-of-contents)
-   [1 Introduction](#1-introduction)
-   [2 Menus](#2-menus)
    -   [2.1 Warps](#21-warps)
    -   [2.2 Scene](#22-scene)
        -   [2.2.1 Collision](#221-collision)
        -   [2.2.2 Free camera](#222-free-camera)
    -   [2.3 Cheats](#23-cheats)
    -   [2.4 Inventory](#24-inventory)
    -   [2.5 Equips](#25-equips)
    -   [2.6 File](#26-file)
    -   [2.7 Macro](#27-macro)
        -   [2.7.1 Settings](#271-settings)
        -   [2.7.2 Virtual controller](#272-virtual-controller)
    -   [2.8 Watches](#28-watches)
    -   [2.9 Debug](#29-debug)
    -   [2.10 Settings](#210-settings)
-   [3 VC issues](#3-vc-issues)
-   [4 Issues with savestates](#4-issues-with-savestates)
    -   [4.1 Dangling pointers](#41-dangling-pointers)
    -   [4.2 Corruptions](#42-corruptions)
    -   [4.3 Graphics](#43-graphics)
    -   [4.4 Audio](#44-audio)
-   [5 About frame advancing and recording](#5-about-frame-advancing-and-recording)
    -   [5.1 Room loading](#51-room-loading)
    -   [5.2 Ocarina notes](#52-ocarina-notes)
    -   [5.3 Ocarina input](#53-ocarina-input)
    -   [5.4 RNG seeds](#54-rng-seeds)

## 1 Introduction
*Note: If you're using gz on the Wii VC, you should read the
[VC issues](#3-vc-issues) section to find out about the differences in the VC
version.*

The main interface for accessing the provided tools is the utility menu. By
default, this menu is brought up by pressing `R + L`, but this button
combination can be changed (see [2.10 Settings](#210-settings)). Use the D-Pad
to navigate the menu, and L to make a selection. For a description of each of
the submenus, see their respective section below.

Beyond the tools provided by the utility menu there is also;

-   **An input display.** The two numbers represent the x and y coordinate of
    the control stick. The button icons that appear represent the buttons that
    are pressed on the controller. *Enabled by default.*
-   **A lag counter.** Displays lag by subtracting the number of game frames
    passed from the game's vertical interrupt counter. Displayed in units of
    frames (60Hz, default), or seconds. *Disabled by default.*
-   **A timer.** Measures real-time using the CPU counter. *Disabled by
    default.*
-   **_Various button-activated commands._**

These features can be configured from the settings menu (see
[2.10 Settings](#210-settings)).

## 2 Menus

### 2.1 Warps
The **places** menu provides a list of all scenes and their respective
entrances, grouped into eight categories. Selecting a scene with multiple
entrances will show a list of all entrances for that scene. Selecting an
entrance will instantly warp you to that entrance. Scenes with only one
entrance will warp you to that entrance when selected, without showing an
entrance list. If you want to warp to a specific entrance index, you can enter
that index in the warps menu and select **warp**. The **age** and **cutscene**
options specify which age Link will be at when performing a warp, and which
cutscene should be played for that scene. These options apply to both the
places menu and warping using an entrance index.

**clear cs pointer** will point the cutscene pointer to an empty cutscene,
which is useful for preventing certain wrong warps from crashing. The bottom of
the warps menu shows information about the game's current warp parameters.

**_Warning:_** Attempting to load a beta scene will cause a crash on all but
the debug version of Ocarina of Time, which is currently unsupported.

**_Warning:_** Starting a game (with a warp or scene loading command) when no
file data is loaded (i.e. from the N64 logo and, to a lesser extent, the file
select menu) will cause *undefined behavior*.

### 2.2 Scene
Selecting **explorer** will bring up the scene explorer, which shows an overlay
of the current scene. Use the D-Pad up and down to navigate forwards and
backwards through the scene, and D-Pad left and right to rotate the view. While
holding Z, the D-Pad up and down will navigate vertically through the scene,
and D-Pad left and right will move sideways. Use the `explore prev room` and
`explore next room` commands to cycle through the rooms of the scene (bound to
`R + D-Down` and `R + D-Up` by default). Pressing L will teleport Link to the
location and orientation of the crosshair and close the scene explorer.

**set entrance point** sets Link's current position and orientation as the
point where he will respawn after voiding out. **clear flags** and **set
flags** modifies the temporary and permanent flags for the current scene, which
keep track of things such as which chests have been opened, which items have
been collected, which enemies have been defeated, etc. The **load room** option
loads the room with the specified index, if a room which such an index exists
within the current scene. If that room is already the currently loaded room,
it will be unloaded. **teleport slot** selects which
position to save and load when using the teleportation commands (this can also
be bound to a button combination, but is unbound by default). The bottom of the
scene menu shows information about the current scene.

#### 2.2.1 Collision
**show collision**, when enabled, shows the static and dynamic scene collision
in the current scene. The collision polygons are color-coded to show special
properties;

-   **Blue:** Hookshotable surface.
-   **Purple:** Surface with special interaction (ladder, vine, crawlspace, not
    climbable, grabbable).
-   **Red:** Void trigger.
-   **Green:** Load trigger.
-   **Light green:** Surface with special behavior (sand, quicksand, ice, lava,
    jabu wall, damaging wall, no recoil wall, void).
-   **Light yellow:** Slippery slope.
-   **White:** Normal surface.

Note that collision polygons are affected by scene lighting and fog, which can
cause their appearance to be misleading. These effects can be disabled by
turning off the **shaded** option. The collision view **mode** decides how
collision polygons are drawn. The _decal_ setting will draw polygons overlaid
on scene textures, but will not produce any new surfaces (note however that
most emulators do not correctly emulate this behavior). The _surface_ setting
draws collision polygons as their own surfaces, but can produce depth
flickering on existing scene textures. Collision polygons can be configured to
be completely opaque, or see-through with the **translucent** option. Water
boundaries are displayed by default, but can be disabled with the
**waterboxes** option. Enabling **wifreframe** will display lines around the
edges of each polygon. The **reduced** option will reduce potential lag by only
rendering collision polygons with some special property (i.e. colored
polygons). When **auto update** is on, the collision view will update
automatically when a collision view setting is changed, or when the collision
changes. If turned off, the collision view must be manually disabled and
re-enabled in order to update.

The **show colliders** option shows simple collision bodies and damage sources
and sinks. The **translucent** and **shaded** options are identical to the
collision view options with the same names. Hitboxes are also color-coded by
their type;

-   **Red:** Hitboxes; deal damage.
-   **Blue:** Hurtboxes; take damage.
-   **White:** Bumpboxes; pushes and/or get pushed.

The types of colliders to show can be selected with the **hit**, **hurt**, and
**bump** options. It's common for objects to have multiple overlapping hitboxes
of different types.

Enabling **show paths** will display all of the paths that are defined in the
current scene. These are usually waypoints for NPC's. Each point in the path
will be displayed when **points** is enabled, and a line will be drawn between
subsequent points when **lines** is enabled. The path display can be
translucent or opaque as selected by the **translucent** option.

#### 2.2.2 Free camera
The free camera function provides full control of the game's camera. When
enabled, the camera can be controlled with the joystick, C buttons, and Z
trigger. These controls are disabled in the game when controlling the free
camera. Press **lock** to disable the manual camera controls and restore the
normal game controls.

The free camera has two **mode** settings; when set to *camera*, the game's
camera is physically moved, which affects the behavior of in-game objects that
react to the camera's position. When set to *view*, only the graphical
viewpoint is changed. The game's camera then behaves as usual and is disjoint
from the user's view of the scene.

The **behavior** setting decides how the camera moves and how the controls
work;

-   **manual:** The camera does not move by itself. Use the joystick to look
    around, and the C buttons to move. Hold Z to move with the joystick, look
    with C-left and right, and move vertically with C-up and down. The
    **distance min** and **distance max** settings do nothing.
-   **birdseye follow:** The camera automatically looks at Link, and moves
    forward and backward to stay within the specified *distance*. Controls
    are the same as for *manual*.
-   **radial follow:** The camera follows Link from a fixed viewing angle. It
    will move up, down, and sideways to keep Link in focus, and forward and
    backward to stay within the specified *distance*. Use the joystick,
    C-left, and C-right to rotate the viewing angle, and C-up and down to move
    towards and away from the focus point. Hold Z to swap the function of C-up
    and down with the vertical joystick axis.

### 2.3 Cheats
This menu allows toggling the builtin cheats on and off. The following cheats
are available:

-   **energy:** Gives full health.
-   **magic:** Gives full magic.
-   **_various items:_** Sets item amounts to the current capacity of that
    item, or to one if the capacity is zero or unlimited.
-   **small keys:** Sets the number of small keys to one within the current
    dungeon, if any.
-   **rupees:** Sets the rupee amount to the capacity of the current wallet.
-   **nayru's love:** Prevents nayru's love from expiring, if active. If
    nayru's love is not currently active, entering a scene while this cheat is
    enabled will  activate it.
-   **freeze time:** Prevents the current time of day from advancing. Does not
    affect sun's song or the time of day modifiers in the file menu.
-   **no music:** Stops background music from playing.
-   **items usable:** Clears all item restriction flags, allowing all items to
    be used regardless of location.
-   **no minimap:** Keeps the minimap hidden at all times.
-   **isg:** Permanently activates the *infinite sword glitch*.
-   **quick text:** Activates fast scrolling for all textboxes.
-   **no hud:** Hides hearts, magic, buttons, rupees and the minimap overlays.

To undo the effects of the *no music*, *items usable*, *no minimap*, and *no
hud* cheats, turn the cheat off and enter a new scene, or reload the current
scene.

### 2.4 Inventory
The **equipment and items** menu lets you modify your equipment, C-button
items, and passive equipment items. There is also an option to set whether or
not the Giant's Knife has been broken, and whether or not the Biggoron's Sword
has been obtained. Pressing a bottle, trade item, or equipment item will bring
up an item wheel where you can select from the possible items for that slot.
Use the D-Pad left and right to cycle through the items, and the D-Pad up and
down to cycle three items at a time. Capacity equipment items (e.g. quiver,
bomb bag etc.) that are not normally obtainable without using cheats or
glitches are denoted with an asterisk.

The **quest items** menu lets you modify all items on the Quest Status screen,
as well as your energy and magic. There are also options to modify the dungeon
items and small key amount of a specified dungeon. **energy cap.**,
**defense**, **magic cap.**, and **heart pieces** are specified in hexadecimal
for simplicity (from here on denoted with an *h*). For energy capacity, 10h
corresponds to one heart container. The defense checkbox enables or disables
double defense damage reduction, and the field next to it modifies the number
of defense heart containers. For magic capacity, 0h is the normal capacity, 1h
is double magic, 2h is what would be triple magic etc. For heart pieces, 10h
corresponds to one heart piece.

The **amounts** menu lets you modify the ammo of your C-button items, your
magic and energy amount, number of hits left on Giant's Knife, and rupee
amount. The number of Giant's Knife hits left is what decides whether or not
the Giant's Knife / Biggoron's Sword appears to be broken when Link wields it.
Magic, energy, and Giant's Knife hits are specified in hexadecimal. 30h magic
is the max for normal magic capacity, and 60h is the max for double magic
capacity. For energy, 10h corresponds to one heart container. Though the
highest amount of rupees that can be specified is 99999, entering a value
greater than or equal to 65536 will wrap the amount around to zero and on.

### 2.5 Equips
This menu lets you modify your current equips, and the items equipped to your B
and C-buttons. Pressing a piece of equipment that is already equipped will
unequip it. Unequpping a sword or equipping nothing to the B button will
automatically set the current file's swordless flag. **_Warning:_** Unequipping
boots will have strange effects, and usually cause an immediate crash. *Note:*
Changing a C-button equip will not modify the equipped item slot for that
button.

### 2.6 File
The **restore skulltulas** option clears all the gold skulltula flags in the
current file, thus restoring all destroyed gold skulltulas. The gold skulltulas
in the current scene are not affected until the scene is reloaded. **call
navi** sets Navi's advice timer to a value which will make her want to
talk to you. This will not have any effect until you enter an area where Navi
normally appears. **load debug file** loads the default file that used on the
title screen and when starting the first file on the debug version file select
menu. The current time of day can be manually adjusted, or automatically
fast-forwarded to day or night with the corresponding buttons. **epona freed**,
**carpenters freed**, **intro cutscenes**, and **rewards obtained** let's you
set and clear various useful flags in the current file. Pressing the checkmark
will set the pertinent flags such that they have been "completed", and the
cross will clear them (setting them to the state that they were in when the
file was created).

The **timer 1** and **timer 2** options display and modify the state of the two
types of timers in the game. The first timer is used for hot rooms and races,
and the second timer is used for trade items and the Castle Collapse sequence.
The numbers represent the number of seconds left on the timers, and the options
show the current state of the timers. Both of these can be modified manually.
*Note:* When the first timer is set to a heat state, the game will instantly
deactivate it if the current room is not "hot". **_Warning:_** Modifying the
state of the timers can yield strange behavior. In general, it is safest to use
the the *starting* and *stopped* states when doing this.

The **file index** option decides which file the game will be saved to when
saving from the start menu or the Game Over screen. When the file index is set
to FFh, as it is by default on the title screen, saving will have no effect.
There's also a **language** and **z targeting** option, which apply to the
current file.

### 2.7 Macro
This menu provies tools for producing tool-assisted gameplay recordings. Press
**pause** to freeze/unfreeze the game, and **frame advance** to advance the
game one frame at a time, allowing for precise input control. Start recording
your inputs by pressing **record macro**, and play back your recorded inputs by
pressing **play macro**. The **macro frame** control shows the number of the
current frame which your inputs are being recorded to / played back from. You
can edit this value to seek to a specific frame of input, or press
**rewind macro** to seek to frame 0. The total number of input frames recorded
in the current macro is displayed on the right. Press **trim macro** to delete
all recorded inputs after the current macro frame, should you feel that you've
recorded too far.

Use the arrows to select a savestate slot, and save the current state of the
game to that slot by pressing **save state**. You can then return to that state
by pressing **load state**. If you are in macro recording/playback when saving
a state, the frame number will be saved within the state, and the macro will
seek to that frame when loading the state. This allows you to record over
previously recorded inputs, to correct mistakes for example. If there's no
macro frame stored in the state, your macro will not be affected.
**clear state** deletes the state in the current state slot. Some information
about the state saved in the current slot is displayed; the scene in which it
was saved, the size of the state, and the macro frame (if any) on which it was
saved.

**quick record movie** is a convenience option to start recording a macro, save
a state to slot 0 for playing back the macro from the start, and then switch to
slot 1 to use for rerecording. **quick play movie** will start macro playback
from the start and load the state in slot 0.

Macros and savestates can be saved to and loaded from an SD card using **export
macro** / **import macro** and **export state** / **import state**. Pressing
any of these buttons will open the file browser. When importing, pressing the
name of a file in the file browser will load it and return to the macro menu.
When exporting, the filename can be changed by pressing the name field. The
default name for savestates is a number followed by the name of the scene where
the state was saved. The numbering starts at zero and increments if there is
already a file with that number in the current directory. Pressing **clear**
will set the name field to be empty. When the name field is empty, the default
filename is `untitled`. Pressing the name of a file will copy that name to the
name field. Pressing accept will save the file to the current folder in the
file browser with the specified file name. If the file exists, you will be
prompted to overwrite it. New directories can be created by pressing the folder
icon to the left of the name field, inputting a name for the new directory with
the on-screen keyboard, and then pressing the name to confirm. To cancel, use
the return command.

_Note:_ Macros override all controller input by default, but this can be
changed with the **macro input** setting in the settings menu.

_Note:_ States can not be used in the file select menu or on the n64 logo.

_See also:_ [4 Issues with savestates](#4-issues-with-savestates).

#### 2.7.1 Settings
This menu provides advanced settings for savestates and macro recording:

-   **recording settings**: gz implements certain hacks to keep macros in sync
    (See
    [5 About frame advancing and recording](#5-about-frame-advancing-and-recording)
    ). All hacks are enabled by default. Disabling the hacks can cause issues
    and desyncs, and should only be done if required (for example, when
    recording a setup that is sensitive to room loading lag). For such use
    cases, the hacks should not be kept disabled longer than necessary (i.e.
    disable only when recording that particular section).
-   **playback settings**: The **pause when playback ends** setting will
    automatically freeze gameplay when macro playback completes, instead of
    letting the game continue to run. The **start/stop timer automatically**
    setting will start or stop the timer when gameplay is unpaused or paused
    respectively, which can be useful for timing macros especially when
    combined with **pause when playback ends**.
-   **game settings**: The **wii vc camera** setting enables a camera quirk
    that is present on the Wii VC versions of the game. This setting can be
    used to sync macros that were made on Wii VC when played back on N64, or
    vice versa. It is enabled by default on Wii VC versions of gz. The
    **ignore state's z-target** option will keep the current z-targeting mode
    when loading states that have a different setting, which is useful for
    practicing with savestates made by someone with different preferences.

#### 2.7.2 Virtual controller
Press the checkbox next to the controller number to override the game's input
for that controller with a virtual controller. When the virtual controller is
enabled, the **plugged in** option decides if the controller should appear to
be connected to the system. The **joystick** controls set the x and y
coordinates of the joystick on the virtual controller, and the **buttons**
controls decide what buttons are held down on the virtual controller.

### 2.8 Watches
This menu lets you add custom RAM watches to observe arbitrary parts of game's
memory in real-time. Pressing the plus icon will add a new watch, and pressing
the cross next to a watch will remove that watch. After adding a watch, enter a
memory address and value type to display the value at that address. These watch
types are available:

-   **u8:** one-byte value, unsigned.
-   **s8:** one-byte value, signed.
-   **x8:** one-byte value, hexadecimal.
-   **u16:** two-byte value, unsigned.
-   **s16:** two-byte value, signed.
-   **x16:** two-byte value, hexadecimal.
-   **u32:** four-byte value, unsigned.
-   **s32:** four-byte value, signed.
-   **x32:** four-byte value, hexadecimal.
-   **f32:** four-byte value, IEEE 754 single-precision floating-point.

Pressing the anchor button next to a watch will release the watch from the
watches menu so that it's always visible, even when the menu is closed. When a
watch is released, a positioning button will appear which lets you change the
position of the watch on the screen. Holding Z when positioning the watch will
move it faster. Pressing the wrench icon next to a watch will open the memory
editor at the address of that watch. The **visible** option can be unchecked to
hide all watches globally.

Watches can be imported from text files on an SD card by pressing the folder
icon. Press a watch file to bring up a list of all watches contained in that
file. Press a watch to import it to your watch list. When you've imported all
the watches you need, press **return** to go back to the watches menu. The
format of watch files is described in the wiki,
[here](https://github.com/glankk/gz/wiki/Watch-File-Syntax).

### 2.9 Debug
_Note: These features are for advanced users. Be careful._

This menu contains various debug features to use for testing;

-   **heap:** Displays information about the game's dynamic memory arena.
-   **display lists:** Displays information about display list usage.
-   **objects:** Shows a list of currently loaded objects, which contain
    graphical assets. The **push** option loads the object file of the given id
    at the end of the list, and the **pop** option unloads the last object in
    the list.
-   **actors:** Browse the currently loaded actors, and spawn new actors.
    Select an actor type and use the arrows to scroll between all the loaded
    actors of that type. The address and id of the selected actor is displayed
    below, as well as the actor variable in that actor instance. The **kill**
    option marks the currently selected actor for deletion, and the **go to**
    option teleports Link to the location of that actor. Pressing **cull zone**
    will display the field of view for the selected actor. When the actor is
    outside of its field of view, it will be culled. To spawn a new actor,
    enter an actor id, variable, the x, y, and z components of the position and
    rotation to spawn the actor at, and press **spawn**. The **fetch from
    link** option loads Link's current position and rotation into the position
    and rotation fields.
-   **flags:** Display and edit saved game flags. The flags are grouped by the
    records they are kept in. Use the arrows to cycle between flag records.
    Press a flag to toggle its state. A red flag is "off", and a green flag is
    "on". The **log** menu displays a list of recent flag events. When a flag
    changes, its record, id, and new value is inserted at the top off the list.
    The **undo** option reverts the effect of the most recent flag event and
    removes it from the log. The **clear** option removes all flag events from
    the log, but does not affect the state of the given flags. _Note:_ The flag
    log only records changes when the log menu is open. If a flag changes and
    then changes back while the log is closed, these changes will not be
    recorded. _Note:_ Scene-related flags only affect the currently loaded
    scene. If a scene flag event that occurred in one scene is undone in a
    different scene, it will not have the desired effect.
-   **memory:** Memory editor. Use the horizontal arrows to cycle between
    memory domains, and the vertical arrows to scroll up and down between
    addresses. Holding Z while scrolling will scroll faster. You can also enter
    an address manually in the address field. To edit memory, select the
    desired data type and press a memory cell to modify it. The leftmost column
    shows the starting address of each row of memory cells. Pressing an address
    will open the watches menu and create a new watch at that address.
-   **rdb:** Remote debugging interface through ED64 USB FIFO. Press **start
    rdb** to attach the debugger and halt the program, and **stop rdb** to
    detach the debugger. Press **break** to hit a breakpoint on the graph
    thread.

### 2.10 Settings
This is where most of the functionality of gz is configured. The **profile**
option selects which profile to save and load settings to and from. When the
game starts, the settings saved to profile zero are automatically loaded, if
any. The appearance of the menu can be configured with the **font** and
**drop shadow** options. Disabling drop shadow can reduce the graphical
computation impact of the menu, but may also reduce readability. The visibility
of the on-screen display elements can be configured with the **input display**,
**log**, **lag counter**, **timer**, and **pause display** options. The screen
position of the utility menu, input display, log, lag counter, and timer can be
configured by their respective positioning buttons. Holding Z when positioning
an element will move it faster. The display unit of the lag counter can be set
to *frames* or *seconds*. **macro input** enables or disables controller input
when macro playback is active. The **break type** option decides how the
*break free* command will function. When the break type is *normal*, the
command will end textboxes, certain events and traditional cutscenes. The
*aggressive* break type will cause the command to also try to reset the camera
and some of Link's state flags. **save settings** and **load settings** will
save and load the current settings to the currently selected profile.
**restore defaults** will restore all saved settings to their default values
(Does not affect saved profiles). If the saved settings were to become
corrupted in such a way that they prevent the game from starting, holding the
Start button when the game is starting will load the default settings instead
of loading profile zero. The following settings are saved:

-   Menu and on-screen displays appearances and settings.
-   Saved positions and the currently selected position slot number.
-   Watches.
-   Command button binds.
-   Activated cheats.
-   Warp menu age, cutscene index, and entrance index.

The **commands** menu lets you bind commands to custom button combinations
and/or activate them manually. Pressing the name of a command will activate
that command, and pressing the button combo in the right column will bind a
button combo to the corresponding command. If you want to unbind a command,
press and keep holding L when starting the binding. A button combo for any
given command can contain at most four buttons. When activating a command with
a button combo, the button combo must explicitly be input the way it appears in
the commands menu. For example, a command with the button combo `R + A` will
only be activated if you press R first and then A, or R and A at the same
time. `A + R`, or `R + B + A` will not activate the corresponding command. If
the set of buttons in one button combo is a subset of those in another button
combo, the former will be overridden by the latter when both are active
simultaneously.

The following commands are available:

-   **show/hide menu:** Opens the utility menu if it's closed, closes it if
    it's opened. *Default: `R + L`*
-   **return from menu:** Returns to the previous menu, as if the *return*
    button was pressed. *Default: `R + D-Left`*
-   **break free:** Attempts to break any effect that removes control of Link.
    *Default: `C-Up + L`* *VC Default: `Start + L`*
-   **levitate:** The classic L to levitate command. *Default: `L`*
-   **fall:** Makes Link fall through the floor, as if there was no floor.
    *Default: `Z + L`*
-   **turbo:** Sets Link's linear velocity to 27. *Default: `unbound`*
-   **noclip:** Toggles noclip, equivalent to the *`L + D-Right`* command in
    the debug version. *Default: `L + D-Right`*
-   **file select:** Returns (or proceeds) to the game's file select menu.
    *Default: `B + L`*
-   **reload scene:** Reloads the current scene, starting from the last scene
    entrance. *Default: `A + L`*
-   **void out:** Reloads the current scene, starting from the last room
    entrance as if Link voided out. *Default: `A + B + L`*
-   **toggle age:** Toggles between Adult and Child Link. Takes effect when
    entering a new area. *Default: `unbound`*
-   **save state:** Save the state of the game to the currently selected state
    slot. *Default: `D-Left`*
-   **load state:** Load the state saved in the currently selected state slot.
    *Default: `D-Right`*
-   **save position:** Saves Link's current position and orientation to the
    current position slot. *Default: `unbound`*
-   **load position:** Teleports Link to the position in the current position
    slot. *Default: `unbound`*
-   **previous state:** Selects the previous savestate slot.
    *Default: `unbound`*
-   **next state:** Selects the next savestate slot. *Default: `unbound`*
-   **previous position:** Selects the previous position slot to be used for
    teleportation. *Default: `unbound`*
-   **next position:** Selects the next position slot to be used for
    teleportation. *Default: `unbound`*
-   **pause/unpause:** Pauses the gameplay, effectively freezing the state of
    the game. If the game is already frozen, resumes gameplay as normal. While
    the game is frozen, a pause icon will appear on the top-left of the screen
    (enabled by default, can be turned off). *Default: `D-Down`*
-   **frame advance:** If the game is frozen by the pause command, advances one
    frame of gameplay. Otherwise, freezes the game as if the pause command was
    activated. *Default: `D-Up`*
-   **record macro:** Start/stop input macro recording. *Default: `unbound`*
-   **play macro:** Start/stop input macro playback. This command can be held
    down to loop the macro. *Default: `unbound`*
-   **collision view:** Toggle the collision view on or off.
    *Default: `unbound`*
-   **hitbox view:** Toggle the hitbox view on or off.
    *Default: `unbound`*
-   **explore prev room:** Loads the previous room while using the scene
    explorer. *Default: `R + D-Down`*
-   **explore next room:** Loads the next room while using the scene explorer.
    *Default: `R + D-Up`*
-   **reset lag counter:** Resets the number of lag frames recorded to zero.
    *Default: `R + B + D-Right`*
-   **toggle watches:** Shows or hides watches globally.
    *Default: `R + D-Right`*
-   **start/stop timer:** Starts the on-screen timer if it is stopped, stops it
    if it's running. *Default: `R + A + D-Left`*
-   **reset timer:** Sets the time of the on-screen timer to zero.
    *Default: `R + B + D-Left`*
-   **start timer:** Starts the on-screen timer if it is stopped.
    *Default: `unbound`*
-   **stop timer:** Stops the on-screen timer if it is running.
    *Default: `unbound`*
-   **reset:** Reset the game, as if the reset button had been pressed.
    *Default: `unbound`*

**_Warning:_** Unbinding the *show/hide menu* or *return from menu* commands,
or binding them to a button combination that will interfere with menu
navigation can make it impossible to use the utility menu. If this happens,
you can restore the default settings by entering the following button sequence:
`D-Up D-Up D-Down D-Down D-Left D-Right D-Left D-Right B A`.

_Note:_ Button combos that interfere with menu navigation for commands that
aren't related to menuing are disabled while the utility menu is active.

## 3 VC Issues
There are some known issues with the Wii VC version of gz;

-   The D-Pad on the Classic/Gamecube Controller is mapped to the L button on
    the Virtual Console. The WAD patcher remaps the D-Pad to be functional
    again, and maps C-Stick Down to L on the Virtual Console to provide access
    to the utility menu.
-   The scene explorer has graphical glitches due to poor emulation.

## 4 Issues with savestates

### 4.1 Dangling pointers
This mainly concerns what's known as the _cutscene pointer_. When executing a
wrong warp, the cutscene pointer will often be a _dangling pointer_, meaning
that it points to memory which is no longer in use, or is now used by something
else. This affects the behavior of wrong warps, because the cutscene data will
now have been replaced by _garbage_.

gz does not save unused memory in states, and this can have some unexpected
side effects when loading a state that had a dangling pointer. The pointer
itself will have the same value, and be exactly the same. However, the memory
that the pointer points to _may not necessarily_ be exactly the same.
Specifically, if the cutscene pointer pointed to garbage, the only guarantee is
that it will still point to garbage, not the exact same garbage. Because of
this, the behavior of wrong warps is not guaranteed.

### 4.2 Corruptions
Some assumptions are made about the state of the game when it is saved. For
example, the scene file is assumed to be largely the same as it appears on the
rom. As such, only a few details actually need to be saved. There are
situations where these assumptions break down; if the scene file had become
corrupted for some reason (e.g. double loading), those corruptions would not
be restored when the scene is loaded again.

### 4.3 Graphics
Graphical dependencies (i.e. object files) take up a lot of space, and saving
them in states would not be practical. Fortunately, they mostly contain static
data that never changes, and can plainly be loaded from the rom when needed.
But there are some graphical effects that modify these files, and gz does not
account for this. Specifically, some dissolving effects are known to modify
textures, which can cause them to look odd after a state load. These issues are
purely aesthetic.

### 4.4 Audio
The state of the audio is well separated from the state of the game, so
restoring the audio state is not necessary for the game to behave correctly
after a state load. The audio state is also quite complex and take up much
space. For these reasons, savestates only save minimal information about the
state of the audio. This has been known to sometimes cause audio bugs, such as
music failing to start playing, or sound effects not playing as they should.

## 5 About frame advancing and recording

### 5.1 Room loading
Room loading normally happens asynchronously. That means the game continues
playing while the room loads, and the room appears whenever it's finished
loading. This usually takes 1 frame, but can take longer, depending on the cpu
and cartridge load. Emulators typically don't emulate cartridge bandwidth
limitations, so the delay is usually 0 frames.

When recording or playing back a macro, room loading behavior is changed to
always load synchronously. Thus the game stops until the room has finished
loading, such that load delay variations can not cause macros to desync.

### 5.2 Ocarina notes
When an ocarina song plays on a staff, the notes appearing on the staff are
synced to the audio. The purpose of this is to compensate for potential
differences in lag or framerate, so that the note always appears when you hear
the sound. This poses a problem when frame advancing, because as far as the
game is concerned, frame advancing is essentially lag. The note sync mechanic
is not robust against severe lag, and such lag can cause the staff to simply
stop playing notes, or break in other strange ways.

gz circumvents these issues by disabling the audio sync when frame advancing or
recording/playing a macro. Instead, the game is assumed to run at a constant
framerate with no lag, so that no timing adjustment needs to be done.

### 5.3 Ocarina input
The game's input manager polls the controller at a rate of ~60Hz. Ocarina input
is handled separately from the common game input, and circumstances can cause
it to be delayed into a different controller input window. When this happens,
the input seen by the ocarina on a given frame may be different from that seen
by the rest of the game.

This is a source of potential desyncs in macros, so when playing or recording a
macro, gz ensures that the ocarina input is the same as the common game input
for any given frame.

### 5.4 RNG seeds
The game's Random Number Generator (RNG) is used to decide the outcomes of
random events. The RNG is deterministic, which means that given the same
initial conditions, it will produce the same results. For example, by starting
from a savestate in the graveyard and playing a macro to have Dampé dig a
grave, your reward should be the same every time, as long as the same savestate
and macro is used.

The value of the RNG is predictable given a known starting state. However, the
starting state of the RNG itself (the _seed_), is generally not predictable
(read _pretty damn random_). The RNG is reseeded on every scene load, and from
that point, the future state of the RNG is equally unpredictable.

In order for macros to stay synchronized across scenes, all RNG reseeds are
recorded in the macro file. When an RNG reseed is detected during macro
playback, the stored seed will be used instead of a random value if the
following conditions hold;

-   The state of the RNG is the same as when the seed was recorded (i.e. the
    RNG is synced up until this point).
-   The reseed happens on the same macro frame that it was recorded.

If either of these would fail, a warning is displayed and the RNG is reseeded
as normal with an unpredictable value.
