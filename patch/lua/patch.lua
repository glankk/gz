local arg = {...}
if #arg < 1 then
  print("usage: `patch <rom-file>` " ..
       "(or drag and drop a rom onto the patch script)")
  local line = io.read()
  if line == nil then return end
  arg[1] = line
end
require("lua/rom_table")
for i = 1, #arg do
  print("making patched rom from `" .. arg[i] .. "`")
  local rom = gru.n64rom_load(arg[i])
  local rom_info = rawget(rom_table, rom:crc32())
  if rom_info == nil then
    error("unrecognized rom", 0)
  end
  local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
  local patch = gru.ups_load("ups/gz-" .. rom_id .. ".ups")
  patch:apply(rom)
  rom:save_file("gz-" .. rom_id ..  ".z64")
end
print("done")
print("press enter to continue")
io.read()
