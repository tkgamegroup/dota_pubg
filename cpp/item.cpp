#include "item.h"
#include "character.h"

#include <flame/graphics/image.h>

std::vector<Item> items;

void load_items()
{
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Boots of Speed";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(5.f / 13, 10.f / 15.f, 6.f / 13, 11.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.equipment_info = []()->EquipmentInfo& {
			static EquipmentInfo info { 
				.part = EquipFoot 
			};
			return info;
		};
		item.passive = [](cCharacterPtr character) {
			character->mov_sp += 20;
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Magic Candy";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(0.f / 13, 3.f / 15.f, 1.f / 13, 4.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->gain_exp(character->exp_max);
		};
	}
}

int Item::find(const std::string& name)
{
	if (items.empty())
		load_items();
	for (auto i = 0; i < items.size(); i++)
	{
		if (items[i].name == name)
			return i;
	}
	return -1;
}

const Item& Item::get(uint id)
{
	if (items.empty())
		load_items();
	return items[id];
}
