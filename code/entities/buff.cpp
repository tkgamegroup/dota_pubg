#include <flame/graphics/image.h>

#include "buff.h"
#include "character.h"

void init_buffs()
{
	//for (auto& section : parse_ini_file(Path::get(L"assets\\buffs.ini")).sections)
	//{
	//	auto& buff = buffs.emplace_back();
	//	buff.id = buffs.size() - 1;
	//	buff.name = section.name;
	//	for (auto& e : section.entries)
	//	{
	//		switch (e.key_hash)
	//		{
	//		case "icon_name"_h:
	//			buff.icon_name = e.values[0];
	//			break;
	//		case "icon_tile_coord"_h:
	//			buff.icon_tile_coord = s2t<2, uint>(e.values[0]);
	//			break;
	//		case "interval"_h:
	//			buff.interval = s2t<float>(e.values[0]);
	//			break;
	//		case "description"_h:
	//			buff.description = e.values[0];
	//			break;
	//		case "parameters"_h:
	//			read_parameters(buff.parameter_names, buff.parameters, e.values);
	//			break;
	//		case "passive"_h:
	//			buff.passive.build(e.values);
	//			break;
	//		case "continuous"_h:
	//			buff.continuous.build(e.values);
	//			break;
	//		}
	//	}
	//}

	//for (auto& buff : buffs)
	//{
	//	if (!buff.icon_name.empty())
	//	{
	//		buff.icon_image = graphics::Image::get(buff.icon_name);
	//		if (buff.icon_image)
	//		{
	//			auto tiles = vec2(buff.icon_image->tiles);
	//			if (tiles != vec2(0.f))
	//				buff.icon_uvs = vec4(vec2(buff.icon_tile_coord) / tiles, vec2(buff.icon_tile_coord + 1U) / tiles);
	//		}
	//	}
	//}
}
