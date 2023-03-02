#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

#include "item.h"
#include "character.h"

std::vector<Item> items;

void init_items()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\items.ini")).sections)
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = section.name;
		for (auto& e : section.entries)
		{
			switch (e.key_hash)
			{
			case "icon_name"_h:
				item.icon_name = e.values[0];
				break;
			case "icon_tile_coord"_h:
				item.icon_tile_coord = s2t<2, uint>(e.values[0]);
				break;
			case "type"_h:
				if (e.values[0] == "I")
					item.type = ItemItem;
				else if (e.values[0] == "C")
					item.type = ItemConsumable;
				break;
			case "description"_h:
				item.description = e.values[0];
				break;
			case "parameters"_h:
				read_parameters(item.parameter_names, item.parameters, e.values);
				break;
			case "active"_h:
				item.active.build(e.values);
				break;
			}
		}
	}

	for (auto& item : items)
	{
		if (!item.icon_name.empty())
		{
			item.icon_image = graphics::Image::get(item.icon_name);
			if (item.icon_image)
			{
				auto tiles = vec2(item.icon_image->tiles);
				if (tiles != vec2(0.f))
					item.icon_uvs = vec4(vec2(item.icon_tile_coord) / tiles, vec2(item.icon_tile_coord + 1U) / tiles);
			}
		}
	}
}

int Item::find(const std::string& name)
{
	for (auto i = 0; i < items.size(); i++)
	{
		if (items[i].name == name)
			return i;
	}
	return -1;
}

const Item& Item::get(uint id)
{
	return items[id];
}
