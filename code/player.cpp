#include <flame/graphics/material.h>
#include <flame/universe/components/mesh.h>

#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
#include "entities/collider.h"

void Player::init(EntityPtr _e_town)
{
	e_town = _e_town;

	if (auto collider = e_town->get_component_t<cCircleCollider>(); collider)
	{
		collider->callbacks.clear();
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				character->die("removed"_h);
				town_hp--;
				if (town_hp == 0)
				{

				}
			}
		});
	}

	buildings.clear();
	{
		auto& building = buildings.emplace_back();
		building.info = building_infos.find("Barracks");
		building.lv = 1;
	}
}

Player player1;
Player player2;
