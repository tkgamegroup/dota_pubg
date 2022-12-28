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
				item.icon_name = e.value;
			else if (e.key == "icon_tile_coord")
				item.icon_tile_coord = s2t<2, uint>(e.value);
			else if (e.key == "type")
			{
				if (e.value == "I")
					item.type = ItemItem;
				else if (e.value == "C")
					item.type = ItemConsumable;
			}
			else if (e.key == "description")
				item.description = e.value;
			else if (e.key == "active")
			{
				for (auto& c : SUS::split(e.value, ';'))
					item.active.push_back(Command(c));
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
