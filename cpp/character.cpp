#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

void IdleState::update()
{
	if (!character->passive && character->party != MiddleSide)
	{
		Party enemy_side = (Party)0;
		switch (character->party)
		{
		case LeftSide:
			enemy_side = RightSide;
			break;
		case RightSide:
			enemy_side = LeftSide;
			break;
		}
		if (enemy_side != 0)
		{
			std::vector<cNodePtr> objs;
			sScene::instance()->octree->get_colliding(character->node->g_pos, 15.f, objs, enemy_side);
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
					state->character = character;
					state->action = BattleState::Waiting;
					state->enemies = enemies;
					character->state.reset(state);
					return;
				}
			}
		}
	}
}

void MoveState::update()
{

}

void BattleState::update()
{
	if (target == nullptr)
	{
		if (enemies.empty())
		{
			auto state = new IdleState;
			state->character = character;
			character->state.reset(state);
			return;
		}
		target = enemies.front();
	}

	if (action == Waiting)
		action = Chase;

	if (action == Chase)
	{
		if (distance(character->node->g_pos, target->node->g_pos) <
			(character->radius + target->radius + character->atk_radius) * 0.1f)
		{
			auto ang0 = character->node->get_eul().x;
			auto ang1 = target->node->get_eul().x;
			auto dist1 = ang0 - ang1; if (dist1 < 0.f) dist1 += 360.f;
			auto dist2 = ang1 - ang0; if (dist2 < 0.f) dist2 += 360.f;
			if (min(dist1, dist2) < 60.f)
			{
				character->nav->stop();
				action = Attack;
				attack_counter = 0;
			}
		}
	}

	if (action == Chase)
		character->nav->set_target(target->node->g_pos);

	if (action == Attack)
	{
		if (attack_counter >= character->atk_frames)
		{
			if (distance(character->node->g_pos, target->node->g_pos) <
				(character->radius + target->radius + character->atk_radius) * 0.15f)
			{
				auto ang0 = character->node->get_eul().x;
				auto ang1 = target->node->get_eul().x;
				auto dist1 = ang0 - ang1; if (dist1 < 0.f) dist1 += 360.f;
				auto dist2 = ang1 - ang0; if (dist2 < 0.f) dist2 += 360.f;
				if (min(dist1, dist2) < 60.f)
				{
					auto hp_prev = target->hp;
					target->hp -= min(target->hp, character->atk);
					printf("attack: damage %d, %d -> %d\n", character->atk, hp_prev, target->hp);
				}
			}
			action = Waiting;
		}
		else
			attack_counter++;
	}
}

cCharacter::cCharacter()
{
	state.reset(new IdleState);
	state->character = this;
}

void cCharacter::die()
{

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
