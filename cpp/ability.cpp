#include "ability.h"
#include "character.h"
#include "buff.h"
#include "projectile.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

std::vector<Ability> abilities;

void load_abilities()
{
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Strong Body";
		ability.icon_name = L"assets\\icons\\strength.png";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.passive = [](uint lv, cCharacterPtr caster) {
			caster->hp_max += lv * 100;
			caster->hp_reg += lv;
		};
		ability.show = [](uint lv) {
			ImGui::Text("Increase HP Max by %d and HP Reg by %d", lv * 100, lv);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Sharp Weapon";
		ability.icon_name = L"assets\\icons\\roguelikeitems.png";
		ability.icon_uvs = vec4(7.f / 13, 1.f / 15.f, 8.f / 13, 2.f / 15.f);
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.passive = [](uint lv, cCharacterPtr caster) {
			caster->atk += lv * 10;
		};
		ability.show = [](uint lv) {
			ImGui::Text("Increase ATK by %d", lv * 10);
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Fire Thrower";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magmaspawn lavariver.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.cast_time = 0.2f;
		ability.mp = 500;
		ability.cd = 7.f;
		ability.distance = 8.f;
		ability.active_l = [](uint lv, cCharacterPtr caster, const vec3& target) {
			auto e = get_prefab(L"assets\\effects\\fire.prefab")->copy();
			e->node()->set_pos(vec3(0.f, 1.8f, 0.f));
			caster->entity->add_child(e);
			add_event([e]() {
				e->remove_from_parent();
				return false; 
			}, 0.3f);
			for (auto c : find_characters(caster->node->pos, 8.f, ~caster->faction))
			{
				//		auto hash = "Fire Thrower"_h + (uint)caster;
				//		if (t->add_marker(hash, 0.5f))
				//			caster->inflict_damage(t, 100.f + caster->INT * 5.f, MagicDamage);
			}
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Shield Bash";
		ability.icon_name = L"assets\\icons\\shield_alpha.png";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetEnemy;
		ability.cast_time = 0.5f;
		ability.mp = 500;
		ability.cd = 10.f;
		ability.distance = 5.f;
		ability.active_t = [](uint lv, cCharacterPtr caster, cCharacterPtr target) {
			caster->inflict_damage(target, 50.f, PhysicalDamage);
			target->add_buff(Buff::find("Stun"), 2.f);
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("Smites an enemy unit with your shield, \n"
				"dealing damage base on your strength and stunning it.");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Weapon";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma pulverize.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.cast_time = 0.f;
		ability.mp = 500;
		ability.cd = 10.f;
		ability.active = [](uint lv, cCharacterPtr caster) {
			caster->add_buff(Buff::find("Flame Weapon"), 60.f);
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Shield";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma seizmic.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.cast_time = 0.f;
		ability.mp = 1000;
		ability.cd = 30.f;
		ability.active = [](uint lv, cCharacterPtr caster) {

		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Stinger";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\funguscorrosive spore.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.passive = [](uint lv, cCharacterPtr caster) {
			caster->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				if (linearRand(0U, 99U) < 10)
					target->add_buff(Buff::find("Poisoned"), 10.f, true);
			});
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Roar";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\fungusfungusbite2.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.cast_time = 0.f;
		ability.mp = 1000;
		ability.cd = 10.f;
		ability.cast_check = [](uint lv, cCharacterPtr caster) {
			return (float)caster->hp / (float)caster->hp_max <= 0.5f;
		};
		ability.active = [](uint lv, cCharacterPtr caster) {
			caster->add_buff(Buff::find("Roar"), 12.f);
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Recover";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\mucus trap.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.cast_time = 3.f;
		ability.mp = 500;
		ability.cd = 0.f;
		ability.cast_check = [](uint lv, cCharacterPtr caster) {
			return (float)caster->hp / (float)caster->hp_max <= 0.5f;
		};
		ability.active = [](uint lv, cCharacterPtr caster) {
			caster->set_hp(min(caster->hp + 1000, caster->hp_max));
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Blink";
		ability.icon_name = L"assets\\icons\\old Ancient Beast icons\\Tactical Flight.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.cast_time = 0.f;
		ability.mp = 500;
		ability.distance = 15.f;
		ability.active_l = [](uint lv, cCharacterPtr caster, const vec3& location) {
			teleport(caster, location);
		};
		ability.show = [](uint lv) {
			ImGui::TextUnformatted("");
		};
	}
}

int Ability::find(const std::string& name)
{
	if (abilities.empty())
		load_abilities();
	for (auto i = 0; i < abilities.size(); i++)
	{
		if (abilities[i].name == name)
			return i;
	}
	return -1;
}

const Ability& Ability::get(uint id)
{
	if (abilities.empty())
		load_abilities();
	return abilities[id];
}
