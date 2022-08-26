#include "ability.h"
#include "character.h"

#include <flame/graphics/image.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

std::vector<Ability> abilities;

const Ability& Ability::get(uint id)
{
	assert(id < abilities.size());
	return abilities[id];
}

void load_abilities()
{
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Fire Ball";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma seizmic.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetEnemy;
		ability.mana = 50;
		ability.distance = 5.f;
		ability.cast_time = 0.5f;
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
				target, [](cCharacterPtr t) {

			});
		};
	}
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = "Blink";
		ability.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\Tactical Flight.jpg";
		ability.icon_image = graphics::Image::get(ability.icon_name);
		ability.target_type = TargetLocation;
		ability.mana = 50;
		ability.distance = 5.f;
		ability.cast_time = 0.f;
		ability.active_l = [](cCharacterPtr caster, const vec3& location) {
			teleport(caster, location);
		};
	}
}
