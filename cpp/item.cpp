#include "item.h"
#include "character.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

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
			if (e.key == "icon_name")
				item.icon_name = e.values[0];
			else if (e.key == "icon_tile_coord")
				item.icon_tile_coord = s2t<2, uint>(e.values[0]);
			else if (e.key == "type")
			{
				if (e.values[0] == "I")
					item.type = ItemItem;
				else if (e.values[0] == "C")
					item.type = ItemConsumable;
			}
			else if (e.key == "description")
				item.description = e.values[0];
			else if (e.key == "parameters")
				read_parameters(item.parameter_names, item.parameters, e.values);
			else if (e.key == "active")
				build_command_list(item.active, e.values);
		}
	}

	for (auto& item : items)
	{
		if (!item.icon_name.empty())
		{
			item.icon_image = graphics::Image::get(item.icon_name);
			if (item.icon_image)
			{
				auto tile_size = vec2(item.icon_image->tile_size);
				if (tile_size != vec2(0.f))
					item.icon_uvs = vec4(vec2(item.icon_tile_coord) / tile_size, vec2(item.icon_tile_coord + 1U) / tile_size);
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
