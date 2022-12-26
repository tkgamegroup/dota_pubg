#include "item.h"
#include "character.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

std::vector<Item> items;

void init_items()
{
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Magic Candy";
		item.icon_name = L"assets\\icons\\roguelikeitems.png";
		item.icon_tile_coord = uvec2(0, 3);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->gain_exp(character->exp_max);
		};
		item.show = []() {
			ImGui::TextUnformatted("Gain exp as much as current level");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Berry";
		item.icon_name = L"assets\\icons\\roguelikeitems.png";
		item.icon_tile_coord = uvec2(6, 12);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->set_hp(min(character->hp + 100, character->hp_max));
		};
		item.show = []() {
			ImGui::TextUnformatted("Recover hp by 100");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Mint";
		item.icon_name = L"assets\\icons\\roguelikeitems.png";
		item.icon_tile_coord = uvec2(1, 13);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->set_mp(min(character->mp + 100, character->mp_max));
		};
		item.show = []() {
			ImGui::TextUnformatted("Recover mp by 50");
		};
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
