local arg = {...}
if #arg < 1 then
  print("usage: `make-wad <wad-file>`")
  return
end
wiivc = true
local gzinject = os.getenv("GZINJECT")
if gzinject == nil or gzinject == "" then
  gzinject = "gzinject"
end
local make = loadfile("patch/lua/make.lua")
for i = 1, #arg do
  print("making wad from `" .. arg[i] .. "`")
  print("unpacking wad")
  gru.os_rm("patch/wadextract")
  local _,_,gzinject_result = os.execute(gzinject .. " -a extract -k patch/common-key.bin -w \"" .. arg[i] ..
                                         "\" -d patch/wadextract --verbose")
  if gzinject_result ~= 0 then
    error("unpacking failed", 0)
  end
  print("making rom")
  local rom_info, rom, patched_rom = make("patch/wadextract/content5/rom")
  if rom_info ~= nil then
    print("saving rom")
    patched_rom:save_file("patch/wadextract/content5/rom")
    print("packing wad")
    local rom_id = rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region
    local _,_,gzinject_result = os.execute(gzinject .. " -a pack -k patch/common-key.bin -w patch/gz-" .. rom_id ..
                                           ".wad -d patch/wadextract -i " .. rom_info.title_id .. " -t gz-" .. rom_id ..
                                           " -r 3 --verbose")
    if gzinject_result ~= 0 then
      error("packing failed", 0)
    end
  end
end
print("done")
