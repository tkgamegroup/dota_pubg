#include "../game.h"
#include "../map.h"
#include "ai.h"
#include "ability.h"
#include "character.h"

void cCreepAI::start()
{
	start_pos = character->node->pos;
}

void cCreepAI::update()
{
	if (multi_player != SinglePlayer && multi_player != MultiPlayerAsHost)
		return;

	auto attack_closest = [this]() {
		bool found = false;
		if (character->action == ActionNone || character->action == ActionMove)
		{
			if (character->search_timer <= 0.f)
			{
				auto enemies = find_characters(~character->faction, character->node->pos, 5.f);
				if (!enemies.empty())
				{
					new CharacterCommandAttackTarget(character, enemies[0]);
					found = true;
				}
				character->search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
			}
		}
		return found;
	};

	switch (type)
	{
	case CreepCamp:
		if (distance(character->node->pos, start_pos) < 10.f)
			aggro_timer = 5.f;
		if (aggro_timer > 0.f)
		{
			aggro_timer -= delta_time;
			if (aggro_timer <= 0.f)
			{
				new CharacterCommandMoveTo(character, start_pos);
				flee_timer = 3.f;
			}
		}

		if (flee_timer > 0.f)
			flee_timer -= delta_time;
		switch (character->command->type)
		{
		case "Idle"_h:
			if (flee_timer <= 0.f)
			{
				if (!attack_closest() && linearRand(0U, 600U) < 10)
					new CharacterCommandMoveTo(character, get_map_coord(start_pos + vec3(linearRand(-3.f, +3.f), 0.f, linearRand(-3.f, +3.f))));
			}
			break;
		case "MoveTo"_h:
			if (flee_timer <= 0.f)
				attack_closest();
			break;
		case "AttackTarget"_h:
			if (flee_timer <= 0.f)
			{
				//if (linearRand(0U, 600U) < 10)
				//{
				//	for (auto& ins : character->abilities)
				//	{
				//		if (ins->cd_timer <= 0.f)
				//		{
				//			if (auto& ability = Ability::get(ins->id); character->mp >= ability.get_mp(ins->lv))
				//			{
				//				if (!ability.cast_check || ability.cast_check(ins.get(), character))
				//				{
				//					if (ability.target_type == TargetNull && ability.active)
				//					{
				//						new CharacterCommandCastAbility(character, ins.get());
				//						break;
				//					}
				//					if (ability.target_type == TargetEnemy && ability.active)
				//					{
				//						new CharacterCommandCastAbilityToTarget(character, ins.get(),
				//							((CharacterCommandAttackTarget*)character->command.get())->target.obj);
				//						break;
				//					}
				//				}
				//			}
				//		}
				//	}
				//}
				//else
				attack_closest();
			}
			break;
		}
		break;
	case CreepLane:
		switch (character->command->type)
		{
		case "Idle"_h:
			if (!attack_closest())
				new CharacterCommandMoveTo(character, target_pos);
			break;
		case "MoveTo"_h:
			attack_closest();
			break;
		case "AttackTarget"_h:
			attack_closest();
			break;
		}
		break;
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

