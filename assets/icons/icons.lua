local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

for i, file in pairs(process.inputs(".png")) do
	local tilemap = superfamiconv.convert_tilemap(
		file,
		superfamiconv.config()
			:mode("wsc"):bpp(2)
			:tile_base(1)
			:palette_base(0)
			:no_discard(true) -- This will keep empty tiles.  Its a waste of space but it makes loading the icons' 3x3 tiles significantly simpler
	)
	process.emit_symbol("gfx_color_" .. process.symbol(file), tilemap)
end
