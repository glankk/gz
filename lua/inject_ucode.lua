local text_size = 0x1190
local text_crc =
{
  0xEFBEB724,
  0x60BAFBE4,
}
local text_loc =
{
  0x00060BF0,
  0x00062D20,
  0x00000034,
  0x00000000,
}
local data_size = 0x03F0
local data_crc =
{
  0x717365BF,
  0x746350D0,
}
local data_loc =
{
  0x000AE050,
  0x000A9410,
  0x000011C4,
  0x00001190,
}

local arg = {...}
if #arg ~= 2 then
  io.stderr:write("usage: inject_ucode <dst-file> <src-file>\n")
  return 1
end
local opt_dst = arg[1]
local opt_src = arg[2]

local dst_rom = gru.n64rom_load(opt_dst)
local src_rom = gru.n64rom_load(opt_src)

local version = nil
local text = nil
local data = nil
local text_insert = nil
local data_insert = nil

-- find text insertion point
for i=1,2 do
  local text_search = gru:blob_create()
  text_search:writestring(0, "gspL3DEX2_fifoTextStart")
  text_insert = dst_rom:find(text_search)
  if text_insert ~= nil then break end
  dst_rom, src_rom = src_rom, dst_rom
  opt_dst, opt_src = opt_src, opt_dst
end
if text_insert ~= nil then
  print(string.format("found text insertion point at 0x%08X in %s",
                      text_insert, opt_dst))
else
  io.stderr:write("unable find text insertion point\n")
  return 2
end

-- find data insertion point
local data_search = gru:blob_create()
data_search:writestring(0, "gspL3DEX2_fifoDataStart")
data_insert = dst_rom:find(data_search)
if data_insert ~= nil then
  print(string.format("found data insertion point at 0x%08X in %s",
                      data_insert, opt_dst))
else
  io.stderr:write("unable find data insertion point\n")
  return 2
end

-- include destination locations as search locations in source
text_loc[#text_loc] = text_insert
data_loc[#data_loc] = data_insert

-- find text
for i = 1,#text_loc do
  local loc = text_loc[i]
  local size = text_size
  if src_rom:size() >= loc + size then
    local blob = src_rom:copy(loc, size)
    local blob_crc = blob:crc32()
    for v = 1,#text_crc do
      if blob_crc == text_crc[v] then
        version = v
        break
      end
    end
    if version ~= nil then
      text = blob
      print(string.format("found text at 0x%08X in %s", loc, opt_src))
      break
    end
  end
end
if text == nil then
  io.stderr:write("unable find text chunk\n")
  return 2
end

-- find data
for i = 1,#data_loc do
  local loc = data_loc[i]
  local size = data_size
  local crc = data_crc[version]
  if src_rom:size() >= loc + size then
    local blob = src_rom:copy(loc, size)
    if blob:crc32() == crc then
      data = blob
      print(string.format("found data at 0x%08X in %s", loc, opt_src))
      print(data:readstring(0x0138, 70))
      break
    end
  end
end
if data == nil then
  io.stderr:write("unable find data chunk\n")
  return 2
end

-- insert microcode
print("inserting microcode")
dst_rom:write(text_insert, text)
dst_rom:write(data_insert, data)

-- save
dst_rom:save(opt_dst)

return 0
