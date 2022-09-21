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
		ability.name = "Fire Thrower";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magmaspawn lavariver.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.cast_time = 0.2f;
		ability.mp = 500;
		ability.cd = 7.f;
		ability.distance = 8.f;
		ability.active_l = [](cCharacterPtr caster, const vec3& target) {
			static EntityPtr projectile = nullptr;
			if (!projectile)
			{
				projectile = Entity::create();
				projectile->load(L"assets\\models\\fireball.prefab");
			}
			auto pos = caster->node->pos +
				vec3(0.f, caster->nav_agent->height * 0.7f, 0.f) +
				caster->node->g_rot[2] * 0.3f;
			auto dir = normalize(target.xz() - pos.xz());
			auto ang = degrees(atan2(dir.y, dir.x));
			auto sp = 12.f;
			for (auto i = -2; i < 3; i++)
			{
				auto a = radians(ang + i * 15.f);
				add_projectile(projectile, pos, pos + vec3(cos(a), 0.f, sin(a)) * 8.f, sp, 0.5f, ~caster->faction,
				[caster](cCharacterPtr t) {
					auto hash = "Fire Thrower"_h + (uint)caster;
					if (t->add_marker(hash, 0.5f))
						caster->inflict_damage(t, 100.f + caster->INT * 5.f, MagicDamage);
				}, [](cProjectilePtr pt) {
					pt->collide_radius += 2.5f * delta_time;
					pt->node->set_scl(vec3(pt->collide_radius));
				});
			}
		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Shield Bash";
		ability.icon_name = L"assets\\icons\\abilities\\shield_alpha.png";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetEnemy;
		ability.cast_time = 0.5f;
		ability.mp = 500;
		ability.cd = 10.f;
		ability.distance = 5.f;
		ability.active_t = [](cCharacterPtr caster, cCharacterPtr target) {
			caster->inflict_damage(target, 50.f + caster->STR * 1.2f, PhysicalDamage);
			target->add_buff(Buff::find("Stun"), 2.f);
		};
		ability.show = []() {
			ImGui::TextUnformatted("Smites an enemy unit with your shield, \n"
				"dealing damage base on your strength and stunning it.");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Weapon";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma pulverize.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetNull;
		ability.cast_time = 0.f;
		ability.mp = 500;
		ability.cd = 10.f;
		ability.cast_check = [](cCharacterPtr caster) {
			return caster->equipments[EquipWeapon0].id != -1;
		};
		ability.active = [](cCharacterPtr caster) {
			auto& ins = caster->equipments[EquipWeapon0];
			ins.enchant = Buff::find("Flame Weapon");
			ins.enchant_timer = 60.f;
			caster->stats_dirty = true;
		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Flame Shield";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma seizmic.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetNull;
		ability.cast_time = 0.f;
		ability.mp = 1000;
		ability.cd = 30.f;
		ability.active = [](cCharacterPtr caster) {

		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Stinger";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\funguscorrosive spore.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetNull;
		ability.passive = [](cCharacterPtr caster) {
			caster->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				if (linearRand(0U, 99U) < 10)
					target->add_buff(Buff::find("Poisoned"), 10.f, true);
			});
		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Roar";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\fungusfungusbite2.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetNull;
		ability.cast_time = 0.f;
		ability.mp = 1000;
		ability.cd = 10.f;
		ability.active = [](cCharacterPtr caster) {
			caster->add_buff(Buff::find("Roar"), 12.f);
		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Recover";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\mucus trap.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetNull;
		ability.cast_time = 3.f;
		ability.mp = 500;
		ability.cd = 0.f;
		ability.cast_check = [](cCharacterPtr caster) {
			return (float)caster->hp / (float)caster->hp_max <= 0.5f;
		};
		ability.active = [](cCharacterPtr caster) {
			caster->hp = min(caster->hp + 1000, caster->hp_max);
		};
		ability.show = []() {
			ImGui::TextUnformatted("");
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Blink";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\Tactical Flight.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.cast_time = 0.f;
		ability.mp = 500;
		ability.distance = 15.f;
		ability.active_l = [](cCharacterPtr caster, const vec3& location) {
			teleport(caster, location);
		};
		ability.show = []() {
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
