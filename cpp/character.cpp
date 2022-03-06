#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/systems/scene.h>

void IdleState::update()
{
	if (character->party == RightSide)
	{
		std::vector<cNodePtr> objs;
		sScene::instance()->octree->get_colliding(character->node->g_pos, 15.f, objs, LeftSide);
		if (!objs.empty())
		{
			std::vector<cCharacterPtr> enemies;
			for (auto obj : objs)
			{
				if (auto chr = obj->entity->get_component_t<cCharacter>(); chr)
					enemies.push_back(chr);
			}
			if (!enemies.empty())
			{
				auto state = new BattleState;
				character->state.reset(state);
				state->character = character;
				state->enemies = enemies;
			}
		}
	}
}

void BattleState::update()
{

}

cCharacter::cCharacter()
{
	state.reset(new IdleState);
	state->character = this;
}

void cCharacter::start()
{

}

void cCharacter::update()
{
	state->update();
}

struct cCharacterCreate : cCharacter::Create
{
	cCharacterPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cCharacter;
	}
}cCharacter_create;
cCharacter::Create& cCharacter::create = cCharacter_create;
