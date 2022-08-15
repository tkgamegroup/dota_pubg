#include "item.h"

#include <flame/graphics/image.h>

std::vector<Item> items;

Item& Item::get(uint id)
{
	assert(id < items.size());
	return items[id];
}

void load_items()
{
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Boots of Speed";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(5.f / 13, 10.f / 15, 6.f / 13, 11.f / 15);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.add_mov_sp = 20;
	}
}
