require("lua/rom_table")

local arg = {...}
local rom = gru.n64rom_load(arg[1])
local rom_info = rom_table[rom:crc32()]
if rom_info == nil then return nil end

local gz_version = rom_info.gz_version
print("rom is " ..
      rom_info.game .. "-" .. rom_info.version .. "-" .. rom_info.region)

print("building gz")
local make = os.getenv("MAKE")
if make == nil or make == "" then make = "make" end
local _,_,make_result = os.execute(make ..
                                   " gz-" .. gz_version ..
                                   " gz-" .. gz_version .. "-hooks")
if make_result ~= 0 then error("failed to build gz", 0) end

print("loading file system")
local fs = gru.z64fs_load_blob(rom)

print("patching files")
local mem_patch = gru.gsc_load("gsc/" .. rom_info.data_dir .. "/mem_patch.gsc")
local hooks = gru.gsc_load("hooks/gz/" .. gz_version .. "/gz.gsc")
gsc_list = { }
if rom_info.version == 'iqs' then 
      gsc_list = { mem_patch, hooks }
else 
      local ups_size_patch = gru.gsc_load("gsc/" .. rom_info.data_dir ..
                                    "/ups_size_patch.gsc")
      gsc_list = { mem_patch, ups_size_patch, hooks }
end
local do_hooks = loadfile("lua/hooks.lua")
do_hooks(rom_info, fs, gsc_list)

print("reassembling rom")
local patched_rom = fs:assemble_rom()

print("building ldr")
local gz = gru.blob_load("bin/gz/" .. gz_version .. "/gz.bin")
local payload_rom = fs:prom_tail()
local payload_ram = 0x80400000
local payload_size = gz:size() + 0x50
local _,_,make_result = os.execute(string.format(make .. " clean-ldr && " ..
                                                 make ..  " ldr" ..
                                                 " CPPFLAGS='" ..
                                                 " -DDMA_ROM=0x%08X" ..
                                                 " -DDMA_RAM=0x%08X" ..
                                                 " -DDMA_SIZE=0x%08X'",
                                                 payload_rom,
                                                 payload_ram,
                                                 payload_size))
if make_result ~= 0 then error("failed to build ldr", 0) end

print("inserting payload")
local ldr = gru.blob_load("bin/ldr/ldr.bin")
local old_ldr = patched_rom:copy(0x1000, 0x50)
patched_rom:write(0x1000, ldr)
patched_rom:write(payload_rom, old_ldr)
patched_rom:write(payload_rom + 0x50, gz)
patched_rom:crc_update()

return rom_info, rom, patched_rom
