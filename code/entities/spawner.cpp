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
		for (auto i = 0; i < spawn_number; i++)
		{
			auto character = add_character(prefab_path, node->pos, faction);
			if (character)
			{
				auto ai = character->entity->get_component_t<cAI>();
				if (ai)
				{
					ai->type = unit_type;
					ai->target_pos = unit_target_pos;
				}
			}
		}

		spawn_timer = spawn_interval;
	}
}

struct cSpawnerCreate : cSpawner::Create
{
	cSpawner* operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cSpawner;
	}
}cSpawner_create;
cSpawner::Create& cSpawner::create = cSpawner_create;
