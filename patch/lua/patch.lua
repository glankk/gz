require("lua/rom_table")
local arg = {...}
if #arg < 1 then
  print("usage: `patch <rom-file>`")
  return
end
local rom = gru.n64rom_load(arg[1])
local rom_info = rom_table[rom:crc32()]
if rom_info == nil then
  print("unrecognized rom")
  return 1
end
print("patching rom...")
local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
local patch = gru.ups_load("gz-" .. rom_id .. ".ups")
patch:apply(rom)
rom:save_file("gz-" .. rom_id ..  ".z64")
print("done")
