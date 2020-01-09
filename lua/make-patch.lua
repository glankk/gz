local progname
if wiivc then progname = "make-patch-vc" else progname = "make-patch" end

function usage()
  io.stderr:write("usage: " .. progname .. " [-o <output-rom>] <input-rom>\n")
  os.exit(1)
end

local arg = {...}
local opt_sub
local opt_out
local opt_rom
while arg[1] do
  if arg[1] == "-s" then
    opt_sub = true
    table.remove(arg, 1)
  elseif arg[1] == "-o" then
    opt_out = arg[2]
    if opt_out == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif opt_rom ~= nil then usage()
  else
    opt_rom = arg[1]
    table.remove(arg, 1)
  end
end
if opt_rom == nil then usage() end

local make = loadfile("lua/make.lua")
local rom_info, rom, patched_rom = make(opt_rom)
if rom_info == nil then
  io.stderr:write(progname .. ": unrecognized rom: " .. opt_rom .. "\n")
  return 2
end

local ups = gru.ups_create(rom, patched_rom)
if opt_out ~= nil then
  ups:save(opt_out)
else
  ups:save("ups/" .. rom_info.gz_name .. ".ups")
end

return 0
