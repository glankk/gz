local arg = {...}
local opt_y = false
if arg[1] == "-y" then
  opt_y = true
  table.remove(arg, 1)
end

if #arg < 1 then
  print("usage: `patch[-vc] [-y] <rom-file>...`" ..
        " (or drag and drop a rom onto the patch script)")
  if opt_y then return 0 end
  local line = io.read()
  if line ~= nil and line:sub(1, 1) == "\"" and line:sub(-1, -1) == "\"" then
    line = line:sub(2, -2)
  end
  if line == nil or line == "" then return end
  arg[1] = line
end

require("lua/rom_table")

local n = 0
for i = 1, #arg do
  io.write("making patched rom from `" .. arg[i] .. "`...")
  local rom = gru.n64rom_load(arg[i])
  local rom_info = rawget(rom_table, rom:crc32())
  if rom_info == nil then
    print(" unrecognized rom, skipping")
  else
    local rom_id = rom_info.game .. "-" ..
                   rom_info.version .. "-" ..
                   rom_info.region
    local patch = gru.ups_load("ups/gz-" .. rom_id .. ".ups")
    patch:apply(rom)
    rom:save_file("gz-" .. rom_id ..  ".z64")
    n = n + 1
    print(" done, saved as " .. rom_id .. ".z64")
  end
end

if n == 0 then
  print("no roms were patched")
elseif n == 1 then
  print(n .. " rom was patched")
else
  print(n .. " roms were patched")
end

if not opt_y then
  print("press enter to continue")
  io.read()
end
return 0
