require("patch/lua/rom_table")
local arg = {...}
local rom = gru.n64rom_load(arg[1])
local rom_info = rom_table[rom:crc32()]
if rom_info == nil then
  print("unrecognized rom")
  return 1
end
local rom_id = rom_info.game .. "-" .. rom_info.version
print("rom is " .. rom_id .. "-" .. rom_info.region)
print("loading file system...")
local fs = gru.z64fs_load_blob(rom)
print("patching code file...")
local code_file = fs:get(rom_info.code_ind)
local text_hook = gru.gsc_load(rom_id .. "/text_hook.gsc")
text_hook:shift(-rom_info.code_ram)
text_hook:apply_be(code_file)
fs:replace(rom_info.code_ind, code_file, fs:compressed(rom_info.code_ind))
print("reassembling rom...")
local patched_rom = fs:assemble_rom()
print("building dependencies...")
local _,_,make_result = os.execute("make gz-" .. rom_id)
if make_result ~= 0 then
  error("failed to build gz", 0)
end
local gz = gru.blob_load("bin/gz/" .. rom_id .. "/gz.bin")
local payload_rom = fs:prom_tail()
local payload_ram = 0x80600000 - 0x60
local payload_size = gz:size() + 0x60
local _,_,make_result = os.execute(string.format("make cleanldr && make ldr " .. 
                                                 "CPPFLAGS='-DDMA_ROM=0x%08X -DDMA_RAM=0x%08X -DDMA_SIZE=0x%08X'",
                                                 payload_rom, payload_ram, payload_size))
if make_result ~= 0 then
  error("failed to build ldr", 0)
end
print("inserting payload...")
gru.gsc_load(rom_id .. "/patch/bss_patch.gsc"):apply_be(patched_rom)
local ldr = gru.blob_load("bin/ldr/ldr.bin")
local old_ldr = patched_rom:copy(0x1000, 0x60)
patched_rom:write(0x1000, ldr)
patched_rom:write(payload_rom, old_ldr)
patched_rom:write(payload_rom + 0x60, gz)
patched_rom:crc_update()
print("done")
return rom_info, rom, patched_rom
