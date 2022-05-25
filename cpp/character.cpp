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
	for (auto chr : enemies)
	{
		auto e = chr->entity;
		e->message_listeners.add([this, e](uint h, void*, void*) {
			if (h == "destroyed"_h)
			{
				for (auto& chr : hate_list)
				{
					if (chr->entity == e)
						chr = nullptr;
				}
			}
		});
	}
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
		for (auto it = hate_list.begin(); it != hate_list.end(); )
		{
			if (*it == nullptr)
				it = hate_list.erase(it);
			else
				it++;
		}
		if (hate_list.empty())
		{
			state = StateIdle;
			action = ActionNone;
		}
		else
		{
			if (action == ActionNone)
				target = hate_list[0];

			auto tar_pos = target->node->g_pos;
			auto self_pos = node->g_pos;
			auto dir = tar_pos - self_pos;
			auto len = length(dir);
			dir = normalize(dir);
			auto ang_dist = angle_dist(node->get_eul().x, degrees(atan2(dir.x, dir.z)));

			if (action == ActionNone || action == ActionMove)
			{
				if (ang_dist <= 60.f && len <= atk_distance)
				{
					action = ActionAttack;
				}
				else
				{
					action = ActionMove;
					nav_agent->set_target(tar_pos);
				}
			}
			else if (action == ActionAttack)
			{

			}
		}
	}
		break;
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
