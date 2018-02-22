local arg = {...}
if #arg < 1 then
  print("usage: `patch-wad <wad-file>` " ..
       "(or drag and drop a wad onto the patch script)")
  local line = io.read()
  if line ~= nil and line:sub(1, 1) == "\"" and line:sub(-1, -1) == "\"" then
    line = line:sub(2, -2)
  end
  if line == nil or line == "" then return end
  arg[1] = line
end
local gzinject = os.getenv("GZINJECT")
if gzinject == nil or gzinject == "" then
  gzinject = "gzinject"
end
wiivc = true
require("lua/rom_table")
local n = 0
for i = 1, #arg do
  io.write("making patched wad from `" .. arg[i] .. "`...")
  gru.os_rm("wadextract")
  local _,_,gzinject_result = os.execute(gzinject .. " -a extract -k common-key.bin -w \"" .. arg[i] ..
                                         "\" -d wadextract")
  if gzinject_result ~= 0 then
    error(" unpacking failed", 0)
  end
  local rom = gru.n64rom_load("wadextract/content5/rom")
  local rom_info = rawget(rom_table, rom:crc32())
  if rom_info == nil then
    print(" unrecognized rom, skipping")
  else
    local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
    local patch = gru.ups_load("ups/gz-" .. rom_id .. ".ups")
    patch:apply(rom)
    rom:save_file("wadextract/content5/rom")
    local _,_,gzinject_result = os.execute(gzinject .. " -a pack -k common-key.bin -w gz-" .. rom_id ..
                                           ".wad -d wadextract -i " .. rom_info.title_id .. " -t gz-" .. rom_id ..
                                           " -r 3")
    if gzinject_result ~= 0 then
      error(" packing failed", 0)
    end
    n = n + 1
    print(" done")
  end
end
if n == 0 then
  print("no wads were patched")
elseif n == 1 then
  print(n .. " wad was patched")
else
  print(n .. " wads were patched")
end
print("press enter to continue")
io.read()
