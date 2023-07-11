#include <flame/universe/systems/scene.h>

#include "../game.h"
#include "character.h"
#include "ai.h"
#include "spawner.h"

void cSpawner::start()
{
	spawn_timer = spawn_delay;
}

void cSpawner::update()
{
	if (spawn_timer > 0.f)
		spawn_timer -= delta_time;
	else
	{
		auto info = character_infos.find(character_name);
		if (info)
		{
			for (auto i = 0; i < spawn_number; i++)
			{
				if (auto character = add_character(info, node->pos, faction); character)
				{
					if (auto ai = character->entity->get_component<cAI>(); ai)
					{
						ai->type = unit_type;
						ai->target_pos = unit_target_pos;
					}
				}
			}
		}

		spawn_timer = spawn_interval;
	}
}

struct cSpawnerCreate : cSpawner::Create
{
	cSpawnerPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cSpawner;
	}
}cSpawner_create;
cSpawner::Create& cSpawner::create = cSpawner_create;
