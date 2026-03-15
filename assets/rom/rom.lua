-- pack_rom.lua
-- Called by wf-process during build to pack a tamagochi rom
-- https://wonderful.asie.pl/doc/wf-lua/modules/wf.api.v1.process.html

local process = require("wf.api.v1.process")

for i, file in pairs(process.inputs(".bin")) do
    local data = process.to_data(file)
	process.emit_symbol("tamagochi_rom_" .. process.symbol(file), data)
end
