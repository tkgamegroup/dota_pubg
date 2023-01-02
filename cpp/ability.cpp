#include "ability.h"
#include "character.h"
#include "buff.h"
#include "projectile.h"
#include "effect.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

std::vector<Ability> abilities;
std::vector<Talent> talents;

void init_abilities()
{
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Greate Cleave";
		ability.icon_name = L"assets\\icons\\Greate_Cleave.jpg";
		ability.max_lv = 4;
		//ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
		//	caster->attack_effects.add([ins](cCharacterPtr attacker, cCharacterPtr target, DamageType, uint) {
		//		auto center = attacker->node->pos;
		//		auto ang = attacker->node->eul.x;
		//		for (auto character : find_characters(center, 3.f, ~attacker->faction))
		//		{
		//			if (character == target)
		//				continue;
		//			auto d = character->node->pos - center;
		//			if (abs(angle_diff(ang, -degrees(atan2(d.z, d.x)))) < 60.f)
		//				attacker->inflict_damage(character, (DamageType)attacker->atk_type, attacker->atk * (ins->lv * 25.f / 100.f));
		//		}
		//	});
		//};
		//ability.show = [](AbilityInstance* ins) {
		//	ImGui::Text("Attack will damage nearby enemies by %d%%", ins->lv * 25);
		//};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Vampiric Spirit";
		ability.icon_name = L"assets\\icons\\Vampiric_Spirit.jpg";
		ability.max_lv = 4;
		//ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
		//	caster->attack_effects.add([ins](cCharacterPtr attacker, cCharacterPtr target, DamageType, uint value) {
		//		attacker->set_hp(min(attacker->hp + uint(value * (ins->lv * 10.f / 100.f)), attacker->hp_max));
		//	});
		//};
		//ability.show = [](AbilityInstance* ins) {
		//	ImGui::Text("Restore health based on attack damage by %d%%", ins->lv * 10);
		//};
	}

	for (auto& section : parse_ini_file(Path::get(L"assets\\abilities.ini")).sections)
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "icon_name")
				ability.icon_name = e.values[0];
			else if (e.key == "icon_tile_coord")
				ability.icon_tile_coord = s2t<2, uint>(e.values[0]);
			else if (e.key == "description")
				ability.description = e.values[0];
			else if (e.key == "target_type")
				TypeInfo::unserialize_t(e.values[0], ability.target_type);
			else if (e.key == "cast_distance")
				ability.cast_distance = s2t<float>(e.values[0]);
			else if (e.key == "cast_range")
				ability.cast_range = s2t<float>(e.values[0]);
			else if (e.key == "cast_angle")
				ability.cast_angle = s2t<float>(e.values[0]);
			else if (e.key == "active")
				ability.active.build(e.values);
			else if (e.key == "passive")
				ability.passive.build(e.values);
		}
	}

	for (auto& ability : abilities)
	{
		if (!ability.icon_name.empty())
		{
			ability.icon_image = graphics::Image::get(ability.icon_name);
			if (ability.icon_image)
			{
				auto tile_size = vec2(ability.icon_image->tile_size);
				if (tile_size != vec2(0.f))
					ability.icon_uvs = vec4(vec2(ability.icon_tile_coord) / tile_size, vec2(ability.icon_tile_coord + 1U) / tile_size);
			}
		}
	}

	// Talents
	{
		auto& talent = talents.emplace_back();
		talent.id = abilities.size() - 1;
		talent.name = "Warrior";
		{
			std::vector<uint> layer;
			layer.push_back(Ability::find("Strong Body"));
			layer.push_back(Ability::find("Strong Mind"));
			layer.push_back(Ability::find("Sharp Weapon"));
			layer.push_back(Ability::find("Rapid Strike"));
			layer.push_back(Ability::find("Scud"));
			layer.push_back(Ability::find("Armor"));
			talent.ablilities_list.push_back(layer);
		}
		{
			std::vector<uint> layer;
			layer.push_back(Ability::find("Greate Cleave"));
			layer.push_back(Ability::find("Vampiric Spirit"));
			talent.ablilities_list.push_back(layer);
		}
	}
	{
		auto& talent = talents.emplace_back();
		talent.id = abilities.size() - 1;
		talent.name = "Laya Knight";
		{
			std::vector<uint> layer;
			layer.push_back(Ability::find("Fire Breath"));
			talent.ablilities_list.push_back(layer);
		}
	}
}

int Ability::find(const std::string& name)
{
	for (auto i = 0; i < abilities.size(); i++)
	{
		if (abilities[i].name == name)
			return i;
	}
	return -1;
}

const Ability& Ability::get(uint id)
{
	return abilities[id];
}

int Talent::find(const std::string& name)
{
	for (auto i = 0; i < talents.size(); i++)
	{
		if (talents[i].name == name)
			return i;
	}
	return -1;
}

const Talent& Talent::get(uint id)
{
	return talents[id];
}
