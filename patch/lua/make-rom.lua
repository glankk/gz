local arg = {...}
if #arg < 1 then
  print("usage: `make-rom[-vc] <rom-file>...`")
  return
end
local make = loadfile("patch/lua/make.lua")
for i = 1, #arg do
  print("making rom from `" .. arg[i] .. "`")
  local rom_info, rom, patched_rom = make(arg[i])
  if rom_info == nil then
    print(" unrecognized rom, skipping")
  else
    print("saving rom")
    patched_rom:save_file("patch/" .. rom_info.gz_name ..  ".z64")
  end
end
print("done")
