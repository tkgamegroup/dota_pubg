#include "../game.h"
#include "../map.h"
#include "ai.h"
#include "ability.h"
#include "character.h"

void cAI::start()
{
	start_pos = character->node->pos;
}

void cAI::update()
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
					character->cmd_attack_target(enemies[0]);
					found = true;
				}
				character->search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
			}
		}
		return found;
	};

	switch (type)
	{
	case UnitCampCreep:
		if (distance(character->node->pos, start_pos) < 10.f)
			aggro_timer = 5.f;
		if (aggro_timer > 0.f)
		{
			aggro_timer -= delta_time;
			if (aggro_timer <= 0.f)
			{
				character->cmd_move_to(start_pos);
				flee_timer = 3.f;
			}
		}

		if (flee_timer > 0.f)
			flee_timer -= delta_time;
		switch (character->command)
		{
		case cCharacter::CommandIdle:
			if (flee_timer <= 0.f)
			{
				if (!attack_closest() && linearRand(0U, 600U) < 10)
					character->cmd_move_to(get_map_coord(start_pos + vec3(linearRand(-3.f, +3.f), 0.f, linearRand(-3.f, +3.f))));
			}
			break;
		case cCharacter::CommandMoveTo:
			if (flee_timer <= 0.f)
				attack_closest();
			break;
		case cCharacter::CommandAttackTarget:
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
	case UnitLaneCreep:
		switch (character->command)
		{
		case cCharacter::CommandIdle:
			if (!attack_closest())
				character->cmd_move_to(target_pos);
			break;
		case cCharacter::CommandMoveTo:
			attack_closest();
			break;
		case cCharacter::CommandAttackTarget:
			attack_closest();
			break;
		}
		break;
	case UnitDefenseTower:
		attack_closest();
		break;
	}
}

struct cAICreate : cAI::Create
{
	cAIPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cAI;
	}
}cAI_create;
cAI::Create& cAI::create = cAI_create;

