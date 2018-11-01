if not wiivc then
  rom_table =
  {
    [0xD423E8B0]  = {
                      game      = "oot",
                      version   = "1.0",
                      region    = "j",
                      code_ind  = 27,
                      code_ram  = 0x800110A0,
                    },
    [0xCD16C529]  = {
                      game      = "oot",
                      version   = "1.0",
                      region    = "u",
                      code_ind  = 27,
                      code_ram  = 0x800110A0,
                    },
    [0x26E73887]  = {
                      game      = "oot",
                      version   = "1.1",
                      region    = "j",
                      code_ind  = 27,
                      code_ram  = 0x800110A0,
                    },
    [0x3FD2151E]  = {
                      game      = "oot",
                      version   = "1.1",
                      region    = "u",
                      code_ind  = 27,
                      code_ram  = 0x800110A0,
                    },
    [0x2B2721BA]  = {
                      game      = "oot",
                      version   = "1.2",
                      region    = "j",
                      code_ind  = 27,
                      code_ram  = 0x800116E0,
                    },
    [0x32120C23]  = {
                      game      = "oot",
                      version   = "1.2",
                      region    = "u",
                      code_ind  = 27,
                      code_ram  = 0x800116E0,
                    },
  }
else
  rom_table =
  {
    [0x2B2721BA]  = {
                      game      = "oot",
                      version   = "vc",
                      region    = "j",
                      code_ind  = 27,
                      code_ram  = 0x800116E0,
                      title_id  = "NGZJ"
                    },
    [0x32120C23]  = {
                      game      = "oot",
                      version   = "vc",
                      region    = "u",
                      code_ind  = 27,
                      code_ram  = 0x800116E0,
                      title_id  = "NGZE"
                    },
  }
end
setmetatable(rom_table, {__index = function(t)
  io.write("unrecognized rom. select an action;\n  0. skip\n")
  local keys = {}
  for k,v in pairs(t) do
    keys[#keys + 1] = k
    io.write(string.format("  %d. treat as `%s`\n",
                           #keys,
                           v.game .. "-" .. v.version .. "-" .. v.region))
  end
  io.flush()
  local n = io.read("n")
  local k = keys[n]
  if k == nil then
    return nil
  end
  return t[k]
end})
