local arg = {...}

local rom_info  = arg[1]
local fs        = arg[2]
local gsc_list  = arg[3]

local ovl_tbl_layout =
{
  part      = {
                stride      = 0x001C,
                vrom_start  = 0x0000,
                vrom_end    = 0x0004,
                vram_start  = 0x0008,
                vram_end    = 0x000C,
                size        = 37,
              },
  actor     = {
                stride      = 0x0020,
                vrom_start  = 0x0000,
                vrom_end    = 0x0004,
                vram_start  = 0x0008,
                vram_end    = 0x000C,
                size        = 471,
              },
  state     = {
                stride      = 0x0030,
                vrom_start  = 0x0004,
                vrom_end    = 0x0008,
                vram_start  = 0x000C,
                vram_end    = 0x0010,
                size        = 6,
              },
  map_mark  = {
                stride      = 0x0018,
                vrom_start  = 0x0004,
                vrom_end    = 0x0008,
                vram_start  = 0x000C,
                vram_end    = 0x0010,
                size        = 1,
              },
  play      = {
                stride      = 0x001C,
                vrom_start  = 0x0004,
                vrom_end    = 0x0008,
                vram_start  = 0x000C,
                vram_end    = 0x0010,
                size        = 2,
              },
}

local boot_ind  = rom_info.boot_ind
local boot_vrom = fs:vstart(boot_ind)
local boot_vram = rom_info.boot_ram
local boot_size = fs:vsize(boot_ind)
local code_ind  = rom_info.code_ind
local code_vrom = fs:vstart(code_ind)
local code_vram = rom_info.code_ram
local code_size = fs:vsize(code_ind)
local code_file = fs:get(code_ind)
local ovl_tbls  = rom_info.ovl_tbls

local files =
{
  [code_ind] = code_file
}

local file_gsc_map = { }

local vram_map =
{
  [boot_vram] = {
                  vrom_start  = boot_vrom,
                  vrom_end    = boot_vrom + boot_size,
                  vram_start  = boot_vram,
                  vram_end    = boot_vram + boot_size,
                },
  [code_vram] = {
                  vrom_start  = code_vrom,
                  vrom_end    = code_vrom + code_size,
                  vram_start  = code_vram,
                  vram_end    = code_vram + code_size,
                },
}

for ovl_type, ovl_tbl in pairs(ovl_tbls) do
  local layout = ovl_tbl_layout[ovl_type]

  local p = (ovl_tbl.addr - code_vram) & 0x1FFFFFFF

  local tbl_stride      = ovl_tbl.stride      or layout.stride
  local tbl_vrom_start  = ovl_tbl.vrom_start  or layout.vrom_start
  local tbl_vrom_end    = ovl_tbl.vrom_end    or layout.vrom_end
  local tbl_vram_start  = ovl_tbl.vram_start  or layout.vram_start
  local tbl_vram_end    = ovl_tbl.vram_end    or layout.vram_end
  local tbl_size        = ovl_tbl.size        or layout.size

  for i = 1, tbl_size do
    local vrom_start  = code_file:read32be(p + tbl_vrom_start)
    local vrom_end    = code_file:read32be(p + tbl_vrom_end)
    local vram_start  = code_file:read32be(p + tbl_vram_start)
    local vram_end    = code_file:read32be(p + tbl_vram_end)

    if vram_start ~= 0 then
      vram_map[vram_start] =
      {
        vrom_start  = vrom_start,
        vrom_end    = vrom_end,
        vram_start  = vram_start,
        vram_end    = vram_end,
      }
    end

    p = p + tbl_stride
  end
end

for _, gsc in ipairs(gsc_list) do
  for i = 1, gsc:size() do
    local addr, value = gsc:get(i - 1)
    local vram_addr   = 0x80000000 + (addr & 0x00FFFFFF)

    local obj = nil
    for _, o in pairs(vram_map) do
      if vram_addr >= o.vram_start and vram_addr < o.vram_end then
        obj = o
        break
      end
    end

    addr = addr - (obj.vram_start & 0x00FFFFFF)

    local file_idx = fs:vat(obj.vrom_start)
    local file_gsc = file_gsc_map[file_idx]

    if file_gsc == nil then
      file_gsc = gru.gsc_create()
      file_gsc_map[file_idx] = file_gsc
    end

    file_gsc:insert(file_gsc:size(), addr, value)
  end
end

for file_idx, file_gsc in pairs(file_gsc_map) do
  local file = files[file_idx]

  if file == nil then
    file = fs:get(file_idx)
    files[file_idx] = file
  end

  file_gsc:apply_be(file)
end

for file_idx, file in pairs(files) do
  fs:replace(file_idx, file, fs:compressed(file_idx))
end
