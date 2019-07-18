local arg = {...}
if #arg < 1 then
  print("usage: `make-patch[-vc] <rom-file>...`")
  return
end
local make = loadfile("patch/lua/make.lua")
for i = 1, #arg do
  print("making patch for `" .. arg[i] .. "`")
  local rom_info, rom, patched_rom = make(arg[i])
  if rom_info ~= nil then
    print("saving patch")
    gru.ups_create(rom, patched_rom):save("patch/ups/" ..
                                          rom_info.gz_name .. ".ups")
  end
end
print("done")
