#include "item.h"
#include "character.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

std::vector<Item> items;

void load_items()
{
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Magic Candy";
		item.icon_name = L"assets\\icons\\roguelikeitems.png";
		item.icon_uvs = vec4(0.f / 13, 3.f / 15.f, 1.f / 13, 4.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
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
		item.icon_uvs = vec4(6.f / 13, 12.f / 15.f, 7.f / 13, 13.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->set_hp(min(character->hp + 1000, character->hp_max));
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
		item.icon_uvs = vec4(1.f / 13, 13.f / 15.f, 2.f / 13, 14.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemConsumable;
		item.active = [](cCharacterPtr character) {
			character->set_mp(min(character->mp + 500, character->mp_max));
		};
		item.show = []() {
			ImGui::TextUnformatted("Recover mp by 50");
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
