-- parse arguments
local arg = {...}
local opt_id
local opt_title
local opt_region
local opt_raphnet
local opt_disable_controller_remappings
local opt_wad
while arg[1] do
  if arg[1] == "-i" or arg[1] == "--channelid" then
    opt_id = arg[2]
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-t" or arg[1] == "--channeltitle" then
    opt_title = arg[2]
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "-r" or arg[1] == "--region" then
    opt_region = arg[2]
    table.remove(arg, 1)
    table.remove(arg, 1)
  elseif arg[1] == "--raphnet" then
    opt_raphnet = true
    table.remove(arg, 1)
  elseif arg[1] == "--disable-controller-remappings" then
    opt_disable_controller_remappings = true
    table.remove(arg, 1)
  elseif arg[1] == "-w" then
    opt_wad = arg[2]
    table.remove(arg, 1)
    table.remove(arg, 1)
  else
    break
  end
end
if #arg < 1 then
  print("usage: \n"
        .. "  `make-wad [<gzinject-arg>...] <wad-file>...`\n"
        .. "  `make-wad [<gzinject-arg>...] -w <wad-file> <rom-file>...`")
  return
end

local gzinject = os.getenv("GZINJECT")
if gzinject == nil or gzinject == "" then
  gzinject = "gzinject"
end

wiivc = true
local make = loadfile("patch/lua/make.lua")

for i = 1, #arg do
  local input_rom
  local input_wad
  if opt_wad ~= nil then
    input_rom = arg[i]
    input_wad = opt_wad
    print("making wad from `" .. input_wad .. "` with `" .. input_rom .. "`...")
  else
    input_rom = "patch/wadextract/content5/rom"
    input_wad = arg[i]
    print("making wad from `" .. input_wad .. "`...")
  end
  -- extract wad
  print("unpacking wad")
  gru.os_rm("patch/wadextract")
  local _,_,gzinject_result = os.execute(gzinject ..
                                         " -a extract" ..
                                         " -k patch/common-key.bin" ..
                                         " -d patch/wadextract" ..
                                         " --verbose" ..
                                         " -w \"" .. input_wad .. "\"")
  if gzinject_result ~= 0 then
    error("unpacking failed", 0)
  end
  -- make rom
  print("making rom")
  local rom_info, rom, patched_rom = make(input_rom)
  if rom_info ~= nil then
    print("saving rom")
    patched_rom:save_file("patch/wadextract/content5/rom")
    -- build gzinject pack command string
    local gzinject_cmd = gzinject ..
                         " -a pack" ..
                         " -k patch/common-key.bin" ..
                         " -d patch/wadextract" ..
                         " --verbose"
    if opt_id then
      gzinject_cmd = gzinject_cmd .. " -i \"" .. opt_id .. "\""
    else
      gzinject_cmd = gzinject_cmd .. " -i " .. rom_info.title_id
    end
    if opt_title then
      gzinject_cmd = gzinject_cmd .. " -w \"patch/" .. opt_title .. ".wad\""
      gzinject_cmd = gzinject_cmd .. " -t \"" .. opt_title .. "\""
    else
      gzinject_cmd = gzinject_cmd .. " -w patch/" .. rom_info.gz_name .. ".wad"
      gzinject_cmd = gzinject_cmd .. " -t " .. rom_info.gz_name
    end
    if opt_region then
      gzinject_cmd = gzinject_cmd .. " -r \"" .. opt_region .. "\""
    else
      gzinject_cmd = gzinject_cmd .. " -r 3"
    end
    if opt_raphnet then
      gzinject_cmd = gzinject_cmd .. " --raphnet"
    end
    if opt_disable_controller_remappings then
      gzinject_cmd = gzinject_cmd .. " --disable-controller-remappings"
    end
    -- execute
    print("packing wad")
    local _,_,gzinject_result = os.execute(gzinject_cmd)
    if gzinject_result ~= 0 then
      error("packing failed", 0)
    end
  end
end

print("done")
