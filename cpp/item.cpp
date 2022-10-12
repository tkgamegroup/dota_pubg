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
		item.name = "Knight Helm";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(4.f / 13, 9.f / 15.f, 5.f / 13, 10.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipHead;
		item.passive = [](cCharacterPtr character) {
			character->phy_def += 4;
		};
		item.show = []() {
			ImGui::TextUnformatted("+4 PHY DEF");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Knight Armor";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(9.f / 13, 9.f / 15.f, 10.f / 13, 10.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipChest;
		item.passive = [](cCharacterPtr character) {
			character->phy_def += 5;
		};
		item.show = []() {
			ImGui::TextUnformatted("+5 PHY DEF");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Knight Trousers";
		item.icon_name = L"assets\\icons\\items\\iron_trousers.png";
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipLeg;
		item.passive = [](cCharacterPtr character) {
			character->phy_def += 4;
		};
		item.show = []() {
			ImGui::TextUnformatted("+4 PHY DEF");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Knight Boots";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(8.f / 13, 10.f / 15.f, 9.f / 13, 11.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipFoot;
		item.passive = [](cCharacterPtr character) {
			character->mov_sp += 5;
			character->phy_def += 2;
		};
		item.show = []() {
			ImGui::TextUnformatted("+5 MOV SP\n+2 PHY DEF");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Boots of Speed";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(5.f / 13, 10.f / 15.f, 6.f / 13, 11.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipFoot;
		item.passive = [](cCharacterPtr character) {
			character->mov_sp += 20;
		};
		item.show = []() {
			ImGui::TextUnformatted("+20 MOV SP");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Straight Sword";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
		item.icon_uvs = vec4(1.f / 13, 7.f / 15.f, 2.f / 13, 8.f / 15.f);
		item.icon_image = graphics::Image::get(item.icon_name);
		item.type = ItemEquipment;
		item.sub_category = EquipWeapon0;
		item.passive = [](cCharacterPtr character) {
			character->atk_type = PhysicalDamage;
			character->atk += 10 + character->STR;
		};
		item.show = []() {
			ImGui::TextUnformatted("Physical\n+10 ATK\nSTR Fix 1.0");
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
		item.show = []() {
			ImGui::TextUnformatted("Gain exp as much as current level");
		};
	}
	{
		auto& item = items.emplace_back();
		item.id = items.size() - 1;
		item.name = "Berry";
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
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
		item.icon_name = L"assets\\icons\\items\\roguelikeitems.png";
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
