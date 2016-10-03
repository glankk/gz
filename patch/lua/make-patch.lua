local arg = {...}
if #arg < 1 then
  print("usage: `make-patch <rom-file>`")
  return
end
local make = loadfile("patch/lua/make.lua")
local rom_info, rom, patched_rom = make(arg[1])
print("saving patch")
local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
gru.ups_create(rom, patched_rom):save("patch/gz-" .. rom_id .. ".ups")
