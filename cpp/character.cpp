#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

std::vector<cCharacterPtr> cCharacter::find_enemies(float radius)
{
	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(node->g_pos, radius ? radius : 15.f, objs, CharacterTag);
	std::vector<cCharacterPtr> enemies;
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && chr->faction != faction)
			enemies.push_back(chr);
	}
	return enemies;
}

void cCharacter::enter_move_state(const vec3& pos)
{
	state = StateMove;
	action = ActionNone;
	nav_agent->set_target(pos);
}

void cCharacter::enter_battle_state(const std::vector<cCharacterPtr>& enemies)
{
	state = StateBattle;
	action = ActionNone;
	hate_list = enemies;
}

void cCharacter::die()
{
	if (dead)
		return;
}

void cCharacter::start()
{
	auto e = entity;
	while (e)
	{
		if (armature = e->get_component_t<cArmature>(); armature)
			break;
		if (!e->children.empty())
			e = e->children[0].get();
	}
}

void cCharacter::update()
{
	switch (state)
	{
	case StateIdle:
		break;
	case StateMove:
		if (length(nav_agent->desire_velocity()) <= 0.f)
		{
			nav_agent->stop();
			state = StateIdle;
			action = ActionNone;
		}
		else
			action = ActionMove;
		break;
	case StateBattle:
	{

		//if (target == nullptr)
		//{
		//	if (enemies.empty())
		//	{
		//		auto state = new IdleState;
		//		state->character = character;
		//		character->state.reset(state);
		//		return;
		//	}
		//	target = enemies.front();
		//}

		//if (action == Waiting)
		//	action = Chase;

		//if (action == Chase)
		//{
		//	if (distance(character->node->g_pos, target->node->g_pos) <
		//		(character->radius + target->radius + character->atk_radius) * 0.1f)
		//	{
		//		auto ang0 = character->node->get_eul().x;
		//		auto ang1 = target->node->get_eul().x;
		//		auto dist1 = ang0 - ang1; if (dist1 < 0.f) dist1 += 360.f;
		//		auto dist2 = ang1 - ang0; if (dist2 < 0.f) dist2 += 360.f;
		//		if (min(dist1, dist2) < 60.f)
		//		{
		//			character->nav_agent->stop();
		//			action = Attack;
		//			attack_counter = 0;
		//		}
		//	}
		//}

		//if (action == Chase)
		//	character->nav_agent->set_target(target->node->g_pos);

		//if (action == Attack)
		//{
		//	if (attack_counter >= character->atk_frames)
		//	{
		//		if (distance(character->node->g_pos, target->node->g_pos) <
		//			(character->radius + target->radius + character->atk_radius) * 0.15f)
		//		{
		//			auto ang0 = character->node->get_eul().x;
		//			auto ang1 = target->node->get_eul().x;
		//			auto dist1 = ang0 - ang1; if (dist1 < 0.f) dist1 += 360.f;
		//			auto dist2 = ang1 - ang0; if (dist2 < 0.f) dist2 += 360.f;
		//			if (min(dist1, dist2) < 60.f)
		//			{
		//				auto hp_prev = target->hp;
		//				target->hp -= min(target->hp, character->atk);
		//				printf("attack: damage %d, %d -> %d\n", character->atk, hp_prev, target->hp);
		//			}
		//		}
		//		action = Waiting;
		//	}
		//	else
		//		attack_counter++;
		//}
	}
		break;
	}
	switch (action)
	{

	}

	if (armature)
	{
		switch (action)
		{
		case ActionNone:
			armature->play("idle"_h);
			break;
		case ActionMove:
			armature->play("run"_h);
			break;
		case ActionAttack:
			armature->play("attack"_h);
			break;
		}
	}
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
