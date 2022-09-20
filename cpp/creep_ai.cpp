#include "creep_ai.h"
#include "character.h"

#include <flame/universe/components/node.h>

void cCreepAI::start()
{
	start_pos = character->node->pos;
}

void cCreepAI::update()
{
	if (distance(character->node->pos, start_pos) > 10.f)
	{

	}
	if (character->action == ActionNone)
	{
		if (character->search_timer <= 0.f)
		{
			auto enemies = get_characters(character->node->pos, 5.f, ~character->faction);
			if (!enemies.empty())
				new CommandAttackTarget(character, enemies[0]);
			character->search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
		}
	}
}

struct cCreepAICreate : cCreepAI::Create
{
	cCreepAIPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cCreepAI;
	}
}cCreepAI_create;
cCreepAI::Create& cCreepAI::create = cCreepAI_create;

