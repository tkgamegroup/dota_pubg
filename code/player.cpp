#include <flame/graphics/material.h>
#include <flame/universe/components/mesh.h>

#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
#include "entities/collider.h"

void BuildingInstance::add_training(const TrainingAction* action, int number)
{
	auto it = std::find_if(trainings.begin(), trainings.end(), [&](const auto& i) {
		return i.action == action;
	});
	if (it == trainings.end())
	{
		it = trainings.emplace(trainings.end(), Training());
		it->action = action;
		it->timer = action->time;
		it->number = number;
	}
	else
	{
		if (number == -1)
			it->number = -1;
		else
			it->number += number;
	}
	trainings_changed_frame = frames;
}

void BuildingInstance::remove_training(const TrainingAction* action)
{
	for (auto it = trainings.begin(); it != trainings.end(); it++)
	{
		if (it->action == action)
		{
			if (it->timer < it->action->time)
			{
				player->blood += it->action->cost_blood;
				player->bones += it->action->cost_bones;
				player->soul_sand += it->action->cost_soul_sand;
			}
			trainings.erase(it);
			return;
		}
	}
	trainings_changed_frame = frames;
}

void Player::init(EntityPtr _e_town)
{
	e_town = _e_town;

	if (auto collider = e_town->get_component_t<cCircleCollider>(); collider)
	{
		collider->callbacks.clear();
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				character->die("removed"_h);
				town_hp--;
				if (town_hp == 0)
				{

				}
			}
		});
	}

	if (auto e_spawn_node = e_town->find_child("spawn_node"); e_spawn_node)
		spawn_node = e_spawn_node->node();

	buildings.clear();
	add_building(building_infos.find("Barracks"), 1);
}

void Player::add_building(const BuildingInfo* info, int lv)
{
	auto& building = buildings.emplace_back();
	building.player = this;
	building.info = info;
	building.lv = lv;
}

void Player::update()
{
	for (auto& b : buildings)
	{
		for (auto& t : b.trainings)
		{
			if (t.timer > 0.f)
			{
				if (t.timer == t.action->time)
				{
					if (blood >= t.action->cost_blood &&
						bones >= t.action->cost_bones &&
						soul_sand >= t.action->cost_soul_sand)
					{
						blood -= t.action->cost_blood;
						bones -= t.action->cost_bones;
						soul_sand -= t.action->cost_soul_sand;
					}
					else
						continue;
				}
				t.timer -= delta_time;
				if (t.timer <= 0.f)
				{
					if (t.number > 0)
						t.number--;
					t.timer = t.action->time;
					if (spawn_node)
					{
						auto unit_info = unit_infos.find(t.action->name);
						if (unit_info)
						{
							if (auto character = add_character(unit_info->prefab_name, spawn_node->global_pos(), faction); character)
							{
								if (auto ai = character->entity->get_component_t<cAI>(); ai)
								{
									ai->type = UnitLaneCreep;
									ai->target_pos = target_pos;
								}
							}
						}
					}
				}
			}
		}
		for (auto it = b.trainings.begin(); it != b.trainings.end();)
		{
			if (it->number == 0)
			{
				it = b.trainings.erase(it);
				b.trainings_changed_frame = frames;
			}
			else
				it++;
		}
	}
}

Player player1;
Player player2;
