-- parse arguments
local arg = {...}
local opt_id
local opt_title
local opt_region
local opt_raphnet
local opt_disable_controller_remappings
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
  else
    break
  end
end
if #arg < 1 then
  print("usage: `make-wad [<gzinject-arg>...] <wad-file>...`")
  return
end

local gzinject = os.getenv("GZINJECT")
if gzinject == nil or gzinject == "" then
  gzinject = "gzinject"
end

wiivc = true
local make = loadfile("patch/lua/make.lua")

for i = 1, #arg do
  -- extract wad
  print("making wad from `" .. arg[i] .. "`...")
  print("unpacking wad")
  gru.os_rm("patch/wadextract")
  local _,_,gzinject_result = os.execute(gzinject ..
                                         " -a extract" ..
                                         " -k patch/common-key.bin" ..
                                         " -d patch/wadextract" ..
                                         " --verbose" ..
                                         " -w \"" .. arg[i] .. "\"")
  if gzinject_result ~= 0 then
    error("unpacking failed", 0)
  end
  -- make rom
  print("making rom")
  local rom_info, rom, patched_rom = make("patch/wadextract/content5/rom")
  if rom_info ~= nil then
    print("saving rom")
    patched_rom:save_file("patch/wadextract/content5/rom")
    -- build gzinject pack command string
    local rom_id = rom_info.game .. "-" ..
                   rom_info.version .. "-" ..
                   rom_info.region
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
      gzinject_cmd = gzinject_cmd .. " -w patch/gz-" .. rom_id .. ".wad"
      gzinject_cmd = gzinject_cmd .. " -t gz-" .. rom_id
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
