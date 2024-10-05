function usage()
  io.stderr:write("usage: make-wad [<gzinject-arg>...] [--no-homeboy]"
                  .. " [-m <input-rom>] [-o <output-wad>] <input-wad>\n")
  os.exit(1)
end

-- parse arguments
local arg = {...}
local opt_id
local opt_title
local opt_keyfile = "common-key.bin"
local opt_region
local opt_directory = "wadextract"
local opt_raphnet
local opt_disable_controller_remappings
local opt_nohb
local opt_rom
local opt_out
local opt_wad
while arg[1] do
  if arg[1] == "-i" or arg[1] == "--channelid" then
    opt_id = arg[2]
    if opt_id == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-t" or arg[1] == "--channeltitle" then
    opt_title = arg[2]
    if opt_title == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-k" or arg[1] == "--key" then
    opt_keyfile = arg[2]
    if opt_keyfile == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-r" or arg[1] == "--region" then
    opt_region = arg[2]
    if opt_region == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-d" or arg[1] == "--directory" then
    opt_directory = arg[2]
    if opt_directory == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "--raphnet" then
    opt_raphnet = true
    table.remove(arg, 1)
  elseif arg[1] == "--disable-controller-remappings" then
    opt_disable_controller_remappings = true
    table.remove(arg, 1)
  elseif arg[1] == "--no-homeboy" then
    opt_nohb = true
    table.remove(arg, 1)
  elseif arg[1] == "-m" then
    opt_rom = arg[2]
    if opt_rom == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-o" then
    opt_out = arg[2]
    if opt_out == nil then usage() end
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif opt_wad ~= nil then usage()
  else
    opt_wad = arg[1]
    table.remove(arg, 1)
  end
end
if opt_wad == nil then usage() end
if opt_rom == nil then opt_rom = opt_directory .. "/content5/rom" end

local gzinject = os.getenv("GZINJECT")
if gzinject == nil or gzinject == "" then gzinject = "gzinject" end

wiivc = true
require("lua/rom_table")
local make = loadfile("lua/make.lua")

-- extract wad
gru.os_rm(opt_directory)
local gzinject_cmd = gzinject ..
                     " -a extract" ..
                     " -k \"" .. opt_keyfile .. "\"" ..
                     " -d \"" .. opt_directory .. "\"" ..
                     " --verbose" ..
                     " -w \"" .. opt_wad .. "\""
local _,_,gzinject_result = os.execute(gzinject_cmd)
if gzinject_result ~= 0 then return gzinject_result end

-- make rom
local rom_info, rom, patched_rom = make(opt_rom)
if rom_info == nil then
  io.stderr:write("make-wad: unrecognized rom: " .. opt_rom .. "\n")
  return 2
end
patched_rom:save_file(opt_directory .. "/content5/rom")

-- check vc version
local vc = gru.blob_load(opt_directory .. "/content1.app")
local vc_version = vc_table[vc:crc32()]

-- make homeboy
if not opt_nohb and vc_version ~= nil then
  print("building homeboy")
  local make = os.getenv("MAKE")
  if make == nil or make == "" then make = "make" end
  local _,_,make_result = os.execute("(cd homeboy && " .. make ..
                                     " hb-" .. vc_version .. ")")
  if make_result ~= 0 then error("failed to build homeboy", 0) end
end

-- build gzinject pack command string
local gzinject_cmd = gzinject ..
                     " -a pack" ..
                     " -k \"" .. opt_keyfile .. "\"" ..
                     " -d \"" .. opt_directory .. "\"" ..
                     " -p \"gzi/gz_mem_patch.gzi\"" ..
                     " --verbose"
if opt_id ~= nil then
  gzinject_cmd = gzinject_cmd .. " -i \"" .. opt_id .. "\""
else
  gzinject_cmd = gzinject_cmd .. " -i " .. rom_info.title_id
end
if opt_title ~= nil then
  gzinject_cmd = gzinject_cmd .. " -t \"" .. opt_title .. "\""
else
  gzinject_cmd = gzinject_cmd .. " -t " .. rom_info.gz_name
end
if opt_region ~= nil then
  gzinject_cmd = gzinject_cmd .. " -r \"" .. opt_region .. "\""
else
  gzinject_cmd = gzinject_cmd .. " -r 3"
end
if not opt_nohb and vc_version ~= nil then
  gzinject_cmd = gzinject_cmd ..  " -p \"gzi/hb_" .. vc_version ..
                 ".gzi\" --dol-inject \"homeboy/bin/hb-" ..
                 vc_version .. "/homeboy.bin\" --dol-loading 80300000"
end
if not opt_disable_controller_remappings then
  if opt_raphnet then
    gzinject_cmd = gzinject_cmd .. " -p \"gzi/gz_remap_raphnet.gzi\""
  else
    gzinject_cmd = gzinject_cmd .. " -p \"gzi/gz_remap_default.gzi\""
  end
end
if opt_out ~= nil then
  gzinject_cmd = gzinject_cmd .. " -w \"" .. opt_out .. "\""
elseif opt_title ~= nil then
  gzinject_cmd = gzinject_cmd .. " -w \"" .. opt_title .. ".wad\""
else
  gzinject_cmd = gzinject_cmd .. " -w \"" .. rom_info.gz_name .. ".wad\""
end
-- execute
local _,_,gzinject_result = os.execute(gzinject_cmd)
if gzinject_result ~= 0 then return gzinject_result end

return 0
