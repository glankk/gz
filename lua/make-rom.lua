local progname
if wiivc then progname = "make-rom-vc" else progname = "make-rom" end

function usage()
  io.stderr:write("usage: " .. progname .. " [-o <output-rom>] <input-rom>\n")
  os.exit(1)
end

local arg = {...}
local opt_out
local opt_rom
while arg[1] do
  if arg[1] == "-o" then
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

if opt_out ~= nil then
  patched_rom:save_file(opt_out)
else
  patched_rom:save_file(rom_info.gz_name ..  ".z64")
end

return 0
