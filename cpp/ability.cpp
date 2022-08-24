#include "ability.h"

#include <flame/graphics/image.h>

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
		ability.active = [](cCharacterPtr caster, cCharacterPtr target) {

		};
	}
}
