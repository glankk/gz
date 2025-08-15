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
local patches = { gru.gsc_load("hooks/gz/" .. gz_version .. "/gz.gsc") }
for _,v in pairs(rom_info.patches) do
  patches[#patches + 1] = gru.gsc_load("gsc/" .. rom_info.data_dir .. "/" .. v .. ".gsc")
end
local do_hooks = loadfile("lua/hooks.lua")
do_hooks(rom_info, fs, patches)

print("reassembling rom")
local patched_rom = fs:assemble_rom()

print("inserting payload")
local gz = gru.blob_load("bin/gz/" .. gz_version .. "/gz.bin")
if gz:size() % 16 ~= 0 then gz:resize(gz:size() + 16 - gz:size() % 16) end
local payload_rom = fs:prom_tail()
local payload_ram = 0x80400060 - 0x60
local payload_size = gz:size() + 0x60
local _,_,make_result = os.execute(string.format(make .. " clean-ldr && " ..
                                                 make ..  " ldr" ..
                                                 " CPPFLAGS='-DZ64_VERSION=Z64_OOTIQC" ..
                                                 " -DDMA_ROM=0x%08X" ..
                                                 " -DDMA_RAM=0x%08X" ..
                                                 " -DDMA_SIZE=0x%08X'",
                                                 payload_rom,
                                                 payload_ram,
                                                 payload_size))
if make_result ~= 0 then error("failed to build ldr", 0) end

print("inserting payload")
local ldr = gru.blob_load("bin/ldr/ldr.bin")
local old_ldr = patched_rom:copy(0x1000, 0x60)patched_rom:write(0x1000, ldr)
patched_rom:write(payload_rom, gz)
patched_rom:crc_update()

return rom_info, rom, patched_rom
