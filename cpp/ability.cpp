#include "ability.h"
#include "character.h"
#include "buff.h"

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
		ability.name = "Fire Ball";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma seizmic.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetEnemy;
		ability.cast_time = 0.5f;
		ability.mp = 500;
		ability.cd = 3.f;
		ability.distance = 5.f;
		ability.active_t = [](cCharacterPtr caster, cCharacterPtr target) {
			static EntityPtr projectile = nullptr;
			if (!projectile)
			{
				projectile = Entity::create();
				projectile->load(L"assets\\models\\fireball.prefab");
			}
			add_projectile(projectile, caster->node->pos +
				vec3(0.f, caster->nav_agent->height * 0.7f, 0.f) +
				caster->node->g_rot[2] * 0.3f,
				target, 0.3f,
			[](cCharacterPtr t) {
				t->add_buff(Buff::find("Stun"), 2.f);
			});
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
		ability.distance = 5.f;
		ability.active_t = [](cCharacterPtr caster, cCharacterPtr target) {
			caster->inflict_damage(target, caster->STR);
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
		ability.name = "Blink";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\Tactical Flight.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.cast_time = 0.f;
		ability.mp = 500;
		ability.distance = 5.f;
		ability.active_l = [](cCharacterPtr caster, const vec3& location) {
			teleport(caster, location);
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
