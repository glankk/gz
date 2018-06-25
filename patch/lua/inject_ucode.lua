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
if #arg < 2 then
  print("usage: `inject_ucode <dst-file> <src-file>`")
  return
end
local dst = gru.n64rom_load(arg[1])
local src = gru.n64rom_load(arg[2])

local version = nil
local text = nil
local data = nil
local text_insert = nil
local data_insert = nil

-- find text insertion point
for i=1,2 do
  local text_search = gru:blob_create()
  text_search:writestring(0, "gspL3DEX2_fifoTextStart")
  text_insert = dst:find(text_search)
  if text_insert then break end
  dst, src = src, dst
  arg[1], arg[2] = arg[2], arg[1]
end
if text_insert then
  print(string.format("found text insertion point at 0x%08X in `%s`", text_insert, arg[1]))
else
  print("unable find text insertion point")
  return 1
end

-- find data insertion point
local data_search = gru:blob_create()
data_search:writestring(0, "gspL3DEX2_fifoDataStart")
data_insert = dst:find(data_search)
if data_insert then
  print(string.format("found data insertion point at 0x%08X in `%s`", data_insert, arg[1]))
else
  print("unable find data insertion point")
  return 1
end

-- include destination locations as search locations in source
text_loc[#text_loc] = text_insert
data_loc[#data_loc] = data_insert

-- find text
for i = 1,#text_loc do
  local loc = text_loc[i]
  local size = text_size
  if src:size() >= loc + size then
    local blob = src:copy(loc, size)
    local blob_crc = blob:crc32()
    for v = 1,#text_crc do
      if blob_crc == text_crc[v] then
        version = v
        break
      end
    end
    if version then
      text = blob
      print(string.format("found text at 0x%08X in `%s`", loc, arg[2]))
      break
    end
  end
end
if not text then
  print("unable find text chunk")
  return 1
end

-- find data
for i = 1,#data_loc do
  local loc = data_loc[i]
  local size = data_size
  local crc = data_crc[version]
  if src:size() >= loc + size then
    local blob = src:copy(loc, size)
    if blob:crc32() == crc then
      data = blob
      print(string.format("found data at 0x%08X in `%s`", loc, arg[2]))
      print(data:readstring(0x0138, 70))
      break
    end
  end
end
if not data then
  print("unable find data chunk")
  return 1
end

-- insert microcode
print("inserting microcode")
dst:write(text_insert, text)
dst:write(data_insert, data)

-- save
print("saving rom to `" .. arg[1] .. "`")
dst:save(arg[1])

print("done")
