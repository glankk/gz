## User manual
The bottom of the screen shows an input display. The two numbers are the x and y coordinate of the analog stick.
The buttons on the controller are represented by letters that appear when a button is pressed.
Press L to bring up the utility menu. Use the D-Pad to navigate the menu, and L to make a selection.

The following builtin button-activated commands are available
- Z + L to levitate.
- Z + D-Pad left to save link's position.
- Z + D-Pad right to load link's position.
- Z + D-Pad down, hold to reload the current area.
  Note that opening a file from the file select menu will not update the last entrance, so using this
  command before loading another area will take you to the default entrance.
- Z + D-Pad up, hold to go to the file select menu.
 
These commands are disabled while using the pause / frame advance feature.

### Inventory
The inventory submenu allows you to modify all parts of your inventory. Most of the amount / capacity modifiers
are specified in hexadecimal (h) for simplicity. As such, every 10h of energy capacity corresponds to one full heart.
For defense hearts, 14h is the default for double defense. For magic capacity, 0h is the default, 1h is double magic.
For heart pieces, every 10h corresponds to one heart piece.
Equipment upgrades that are not normally obtainable without cheats or glitches are denoted with an asterisk.
For dungeon keys, ffh means the key amount will not be visible. Note that not all dungeon items are available in some
dungeons, and therefore modifying them will not have any effect.

### Equips
The equips submenu lets you modify what equipment is selected on the equipment screen on the pause menu.
In order for changed equips to take effect, you must pause and unpause.
For the B and c button modifiers, ffh means the button is blank. Other values correspond to their respective items.
The B button can only be blank if the swordless flag is set.
A list of item indices can be found [here](http://wiki.cloudmodding.com/oot/Item_List).

### Misc
The misc submenu has various uncategorized options.
The `reset all gs` option will restore all gold skulltulas that have been killed, everywhere.
The `clear / set scene flags` option will change all permanent and temporary scene flags which keep track of things
like which chests have been opened, which items have been collected, which enemies have been defeated etc.
The `teleport slot` lets you store several positions with the Z + D-Pad left / right commands.
Furhermore there is language selection, pausing, and a frame advance feature.

### Cheats
The cheats submenu lets you toggle the builtin cheats on and off.
Energy / magic / item amounts / rupees will be set to your current maximum capacity.
If an item capacity is at zero, the amount will be set to one.
`nayru's love` will prevent nayru's love from expiring, when active. If nayru's love is not active when
this cheat is enabled, going to a new area will activate it.
`advance time` will speed up the flow of daytime when enabled, even in areas where time does not normally pass.

### Warps
The `warp` option will go to the specified entrance, with link at the specified age.
Note that the entrance index is subject to age/daytime/cutscene modifiers, which may give unexpected results.
Generally, during normal gameplay, selecting the first in a quad of entrances will always give the desired result.
The `clear cutscene pointer` is useful for preventing certain wrong warps from crashing.
A list of entrance indices and where they go can be found [here](http://wiki.cloudmodding.com/oot/Entrance_Table_(Data)).

### Watches
The watches submenu provides a minimal memory watch utility.
Select `+` to add a new watch. Enter an address and a data type, it's value will be displayed to the right.
