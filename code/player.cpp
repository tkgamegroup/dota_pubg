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
		it->duration = action->duration;
		it->timer = it->duration;
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
			if (it->resources_costed)
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

void TownInstance::init(EntityPtr _e, Player* _player, const TownInfo* _info)
{
	e = _e;
	player = _player;
	info = _info;

	hp_max = info->hp_max;
	hp = hp_max;

	if (auto collider = e->get_component_t<cCircleCollider>(); collider)
	{
		collider->callbacks.clear();
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				character->die("removed"_h);
				hp--;
				if (hp == 0)
				{

				}
			}
		});
	}

	if (auto e_spawn_node = e->find_child("spawn_node"); e_spawn_node)
		spawn_node = e_spawn_node->node();

	buildings.clear();
}

void TownInstance::add_building(const BuildingInfo* info, int lv)
{
	auto& building = buildings.emplace_back();
	building.player = player;
	building.info = info;
	building.lv = lv;
}

void TownInstance::add_construction(const ConstructionAction* action)
{
	auto it = std::find_if(constructions.begin(), constructions.end(), [&](const auto& i) {
		return i.action == action;
	});
	if (it == constructions.end())
	{
		it = constructions.emplace(constructions.end(), Construction());
		it->action = action;
		it->duration = action->duration;
		it->timer = it->duration;
	}
	constructions_changed_frame = frames;
}

void TownInstance::remove_construction(const ConstructionAction* action)
{
	for (auto it = constructions.begin(); it != constructions.end(); it++)
	{
		if (it->action == action)
		{
			if (it->resources_costed)
			{
				player->blood += it->action->cost_blood;
				player->bones += it->action->cost_bones;
				player->soul_sand += it->action->cost_soul_sand;
			}
			constructions.erase(it);
			return;
		}
	}
	constructions_changed_frame = frames;
}

void TownInstance::update()
{
	for (auto& c : constructions)
	{
		if (c.timer <= 0.f)
			continue;
		if (!c.resources_costed)
		{
			if (player->blood >= c.action->cost_blood &&
				player->bones >= c.action->cost_bones &&
				player->soul_sand >= c.action->cost_soul_sand)
			{
				player->blood -= c.action->cost_blood;
				player->bones -= c.action->cost_bones;
				player->soul_sand -= c.action->cost_soul_sand;
				c.resources_costed = true;
			}
			else
				continue;
		}
		c.timer -= delta_time;
		if (c.timer <= 0.f)
		{

		}
	}
	for (auto& b : buildings)
	{
		for (auto& t : b.trainings)
		{
			if (t.timer <= 0.f)
				continue;
			if (!t.resources_costed)
			{
				if (player->blood >= t.action->cost_blood &&
					player->bones >= t.action->cost_bones &&
					player->soul_sand >= t.action->cost_soul_sand)
				{
					player->blood -= t.action->cost_blood;
					player->bones -= t.action->cost_bones;
					player->soul_sand -= t.action->cost_soul_sand;
					t.resources_costed = true;
				}
				else
					continue;
			}
			t.timer -= delta_time;
			if (t.timer <= 0.f)
			{
				if (t.number > 0)
					t.number--;
				t.timer = t.duration;
				t.resources_costed = false;
				if (spawn_node)
				{
					if (auto unit_info = unit_infos.find(t.action->name); unit_info)
					{
						if (auto character = add_character(unit_info->prefab_name, spawn_node->global_pos(), player->faction); character)
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

void Player::init(EntityPtr e_town)
{
	{
		auto info = town_infos.find("Vampire");
		town.init(e_town, this, info);
	}
	{
		auto info = building_infos.find("Barracks");
		town.add_building(info, 1);
	}
}

void Player::update()
{
	town.update();
}

Player player1;
Player player2;
