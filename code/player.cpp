#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
#include "entities/collider.h"

const auto TROOP_CX = 4;
const auto TROOP_CY = 4;

void Player::init(EntityPtr e_town)
{
	town_pos = e_town->node()->pos;
	if (auto collider = e_town->get_component_t<cCircleCollider>(); collider)
	{
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
				character->die("removed"_h);
		});
	}

	formation.resize(TROOP_CX * TROOP_CY);

	avaliable_unit_infos.clear();
	if (!unit_infos.infos.empty())
		avaliable_unit_infos.push_back(&unit_infos.infos[0]);
}

void Player::spawn_troop()
{
	troop.clear();

	auto gap = 2.f;
	auto off = town_pos + troop_spawn_off + vec3((TROOP_CX - 1) * gap * 0.5f, 0.f, (TROOP_CY - 1) * gap * 0.5f);
	for (auto y = 0; y < TROOP_CY; y++)
	{
		for (auto x = 0; x < TROOP_CX; x++)
		{
			auto unit_info = formation[y * TROOP_CX + x];
			if (!unit_info)
				continue;
			if (auto character = add_character(unit_info->prefab_name, off + vec3(x * gap, 0.f, y * gap), faction); character)
			{
				if (auto ai = character->entity->get_component_t<cAI>(); ai)
				{
					ai->type = UnitLaneCreep;
					ai->target_pos = troop_target_pos;
				}

				character->entity->message_listeners.add([this, character](uint hash, void*, void*) {
					if (hash == "destroyed"_h)
					{
						std::erase_if(troop, [&](const auto& i) {
							return i == character;
						});
					}
				});
				troop.push_back(character);
			}
		}
	}
}

void Player::remove_troop()
{
	for (auto c : troop)
		c->die("removed"_h);
	troop.clear();
}

Player player1;
Player player2;
