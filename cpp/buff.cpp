#include "buff.h"
#include "character.h"

#include <flame/graphics/image.h>

std::vector<Buff> buffs;

void init_buffs()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\buffs.ini")).sections)
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "icon_name")
				buff.icon_name = e.values[0];
			else if (e.key == "icon_tile_coord")
				buff.icon_tile_coord = s2t<2, uint>(e.values[0]);
			else if (e.key == "interval")
				buff.interval = s2t<float>(e.values[0]);
			else if (e.key == "description")
				buff.description = e.values[0];
			else if (e.key == "parameters")
				read_parameters(buff.parameter_names, buff.parameters, e.values);
			else if (e.key == "passive")
				buff.passive.build(e.values);
			else if (e.key == "continuous")
				buff.continuous.build(e.values);
		}
	}

	for (auto& buff : buffs)
	{
		if (!buff.icon_name.empty())
		{
			buff.icon_image = graphics::Image::get(buff.icon_name);
			if (buff.icon_image)
			{
				auto tile_size = vec2(buff.icon_image->tile_size);
				if (tile_size != vec2(0.f))
					buff.icon_uvs = vec4(vec2(buff.icon_tile_coord) / tile_size, vec2(buff.icon_tile_coord + 1U) / tile_size);
			}
		}
	}
}

int Buff::find(const std::string& name)
{
	for (auto i = 0; i < buffs.size(); i++)
	{
		if (buffs[i].name == name)
			return i;
	}
	return -1;
}

const Buff& Buff::get(uint id)
{
	return buffs[id];
}
