local arg = {...}
if #arg < 1 then
  print("usage: `make-patch <rom-file>`")
  return
end
local make = loadfile("patch/lua/make.lua")
local rom_info, rom, patched_rom = make(arg[1])
print("saving rom")
local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
patched_rom:save_file("patch/gz-" .. rom_id ..  ".z64")
