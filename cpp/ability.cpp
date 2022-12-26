#include "ability.h"
#include "character.h"
#include "buff.h"
#include "projectile.h"
#include "effect.h"

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
		ability.name = "Strong Body";
		ability.icon_name = L"assets\\icons\\strength.png";
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->hp_max += ins->lv * 100;
			caster->hp_reg += ins->lv;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
						"Increase HP Max by %d and HP Reg by %d", ins->lv * 100, ins->lv);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Strong Mind";
		ability.icon_name = L"assets\\icons\\intelligence.png";
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->mp_max += ins->lv * 100;
			caster->mp_reg += ins->lv;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
				"Increase MP Max by %d and MP Reg by %d", ins->lv * 100, ins->lv);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Sharp Weapon";
		ability.icon_name = L"assets\\icons\\roguelikeitems.png";
		ability.icon_tile_coord = uvec2(1, 7);
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->atk += ins->lv * 10;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
						"Increase ATK by %d", ins->lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Rapid Strike";
		ability.icon_name = L"assets\\icons\\roguelikeitems.png";
		ability.icon_tile_coord = uvec2(10, 9);
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->atk_sp += ins->lv * 10;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
						"Increase ATK SP by %d", ins->lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Scud";
		ability.icon_name = L"assets\\icons\\agility.png";
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->mov_sp += ins->lv * 10;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
						"Increase MOV SP by %d", ins->lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Armor";
		ability.icon_name = L"assets\\icons\\roguelikeitems.png";
		ability.icon_tile_coord = uvec2(9, 9);
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->phy_def += ins->lv * 10;
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
						"Increase Phy DEF by %d", ins->lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Grate Cleave";
		ability.icon_name = L"assets\\icons\\Greate_Cleave.jpg";
		ability.max_lv = 4;
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->attack_effects.add([ins](cCharacterPtr attacker, cCharacterPtr target, DamageType, uint) {
				auto center = attacker->node->pos;
				auto ang = attacker->node->eul.x;
				for (auto character : find_characters(center, 3.f, ~attacker->faction))
				{
					if (character == target)
						continue;
					auto d = character->node->pos - center;
					if (abs(angle_diff(ang, -degrees(atan2(d.z, d.x)))) < 60.f)
						attacker->inflict_damage(character, (DamageType)attacker->atk_type, attacker->atk * (ins->lv * 25.f / 100.f));
				}
			});
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
				"Attack will damage nearby enemies by %d%%", ins->lv * 25);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Vampiric Spirit";
		ability.icon_name = L"assets\\icons\\Vampiric_Spirit.jpg";
		ability.max_lv = 4;
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->attack_effects.add([ins](cCharacterPtr attacker, cCharacterPtr target, DamageType, uint value) {
				attacker->set_hp(min(attacker->hp + uint(value * (ins->lv * 10.f / 100.f)), attacker->hp_max));
			});
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Passive\n"
				"Restore health based on attack damage by %d%%", ins->lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Fire Breath";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magmaspawn lavariver.jpg";
		ability.target_type = TargetLocation;
		ability.cast_time = 0.2f;
		ability.mp = 100;
		ability.cd = 12.f;
		ability.distance = 6.f;
		ability.angle = 60.f;
		ability.active_l = [](AbilityInstance* ins, cCharacterPtr caster, const vec3& target) {
			auto node = caster->node;
			auto center = node->pos;
			auto d = target - center;
			auto target_ang = -degrees(atan2(d.z, d.x));
			add_effect(EffectPreset::find("Fire"), center + vec3(0.f, 1.8f, 0.f), vec3(target_ang, 0.f, 0.f), 0.6f);
			for (auto character : find_characters(center, 6.f, ~caster->faction))
			{
				auto d = character->node->pos - center;
				if (abs(angle_diff(target_ang, -degrees(atan2(d.z, d.x)))) < 60.f)
					caster->inflict_damage(character, MagicDamage, ins->lv * 80);
			}
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::Text("Target: Location\n"
						"Range: 6\n"
						"Unleashes a breath of fire in front you that damage enemies by %d", ins->lv * 80);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Shield Bash";
		ability.icon_name = L"assets\\icons\\shield_alpha.png";
		ability.target_type = TargetEnemy;
		ability.cast_time = 0.5f;
		ability.mp = 50;
		ability.cd = 10.f;
		ability.distance = 5.f;
		ability.active_t = [](AbilityInstance* ins, cCharacterPtr caster, cCharacterPtr target) {
			caster->inflict_damage(target, PhysicalDamage, 50.f);
			target->add_buff(Buff::find("Stun"), 2.f);
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("Smites an enemy unit with your shield, \n"
				"dealing damage base on your strength and stunning it.");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Weapon";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma pulverize.jpg";
		ability.cast_time = 0.f;
		ability.mp = 50;
		ability.cd = 10.f;
		ability.active = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->add_buff(Buff::find("Flame Weapon"), 60.f);
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Shield";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma seizmic.jpg";
		ability.cast_time = 0.f;
		ability.mp = 100;
		ability.cd = 30.f;
		ability.active = [](AbilityInstance* ins, cCharacterPtr caster) {

		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Stinger";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\funguscorrosive spore.jpg";
		ability.passive = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				if (linearRand(0U, 99U) < 10)
					target->add_buff(Buff::find("Poisoned"), 10.f, true);
			});
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Roar";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\fungusfungusbite2.jpg";
		ability.cast_time = 0.f;
		ability.mp = 100;
		ability.cd = 10.f;
		ability.cast_check = [](AbilityInstance* ins, cCharacterPtr caster) {
			return (float)caster->hp / (float)caster->hp_max <= 0.5f;
		};
		ability.active = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->add_buff(Buff::find("Roar"), 12.f);
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Recover";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\mucus trap.jpg";
		ability.cast_time = 3.f;
		ability.mp = 50;
		ability.cd = 0.f;
		ability.cast_check = [](AbilityInstance* ins, cCharacterPtr caster) {
			return (float)caster->hp / (float)caster->hp_max <= 0.5f;
		};
		ability.active = [](AbilityInstance* ins, cCharacterPtr caster) {
			caster->set_hp(min(caster->hp + 100, caster->hp_max));
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Blink";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\Tactical Flight.jpg";
		ability.target_type = TargetLocation;
		ability.cast_time = 0.f;
		ability.mp = 50;
		ability.distance = 15.f;
		ability.active_l = [](AbilityInstance* ins, cCharacterPtr caster, const vec3& location) {
			teleport(caster, location);
		};
		ability.show = [](AbilityInstance* ins) {
			ImGui::TextUnformatted("");
		};
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
			layer.push_back(Ability::find("Grate Cleave"));
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
