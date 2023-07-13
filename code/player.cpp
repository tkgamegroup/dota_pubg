#include <flame/graphics/material.h>
#include <flame/universe/components/mesh.h>

#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
#include "entities/collider.h"
#include "entities/town.h"

void BuildingInstance::add_training(const TrainingAction* action, int number, bool new_training)
{
	auto it = new_training ? trainings.end() : std::find_if(trainings.begin(), trainings.end(), [&](const auto& i) {
		return i.action == action;
	});
	if (it == trainings.end())
	{
		auto unit_info = character_infos.find(action->name);
		if (!unit_info)
			return;
		it = trainings.emplace(trainings.end(), Training());
		it->action = action;
		it->unit_info = unit_info;
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

void BuildingInstance::remove_training(uint idx)
{
	if (idx >= trainings.size())
		return;
	auto& training = trainings[idx];
	if (training.resources_costed)
	{
		player->blood += training.action->cost_blood;
		player->bones += training.action->cost_bones;
		player->soul_sand += training.action->cost_soul_sand;
	}
	trainings.erase(trainings.begin() + idx);
	trainings_changed_frame = frames;
}

void TownInstance::init(EntityPtr _e, Player* _player, const TownInfo* _info)
{
	e = _e;
	auto c_town = e->get_component<cTown>();
	if (c_town)
	{
		towns.push_back(c_town);
		c_town->ins = this;
	}
	player = _player;
	info = _info;

	hp_max = info->hp_max;
	hp = hp_max;

	if (auto collider = e->get_component<cCircleCollider>(); collider)
	{
		collider->callbacks.clear();
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				if (auto ai = character->entity->get_component<cAI>(); ai)
				{
					if (ai->target_node == e->get_component<cNode>())
					{
						if (character->faction != player->faction)
						{
							character->die("removed"_h);
							hp--;
							if (hp == 0)
							{

							}
						}
						else
						{
							ai->target_node = nullptr;
							if (spawn_node)
								ai->target_pos = spawn_node->global_pos();
						}
					}
				}
			}
		});
	}

	if (auto e_spawn_node = e->find_child("spawn_node"); e_spawn_node)
		spawn_node = e_spawn_node->get_component<cNode>();

	buildings.clear();
}

uint TownInstance::get_blood_production() const
{
	uint ret = 16;
	for (auto& b : buildings)
	{
		for (auto& r : b.info->resource_production)
		{
			if (r.type == ResourceBlood)
				ret += r.amount * b.number;
		}
	}
	return ret;
}

uint TownInstance::get_bones_production() const
{
	uint ret = 8;
	for (auto& b : buildings)
	{
		for (auto& r : b.info->resource_production)
		{
			if (r.type == ResourceBones)
				ret += r.amount * b.number;
		}
	}
	return ret;
}

uint TownInstance::get_soul_sand_production() const
{
	uint ret = 1;
	for (auto& b : buildings)
	{
		for (auto& r : b.info->resource_production)
		{
			if (r.type == ResourceSoulSand)
				ret += r.amount * b.number;
		}
	}
	return ret;
}

void TownInstance::add_building(const BuildingInfo* info)
{
	auto it = std::find_if(buildings.begin(), buildings.end(), [&](const auto& i) {
		return i.info == info;
	});
	if (it == buildings.end())
	{
		it = buildings.emplace(buildings.end(), BuildingInstance());
		it->player = player;
		it->info = info;
		it->number = 1;
	}
	else
		it->number++;
}

void TownInstance::add_construction(const ConstructionAction* action)
{
	auto it = std::find_if(constructions.begin(), constructions.end(), [&](const auto& i) {
		return i.action == action;
	});
	if (it == constructions.end())
	{
		auto building_info = building_infos.find(action->name);
		if (!building_info)
			return;
		it = constructions.emplace(constructions.end(), Construction());
		it->action = action;
		it->building_info = building_info;
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

bool TownInstance::send_troop(cNodePtr target, const std::vector<std::pair<const CharacterInfo*, uint>>& formation)
{
	auto ok = true;
	for (auto& u : formation)
	{
		auto n = 0;
		for (auto c : troop)
		{
			if (c->info == u.first)
			{
				if (auto ai = c->entity->get_component<cAI>(); ai)
				{
					if (!ai->target_node)
						n++;
				}
			}
		}
		if (n < u.second)
		{
			ok = false;
			break;
		}
	}
	if (ok)
	{
		for (auto& u : formation)
		{
			auto n = u.second;
			if (n > 0)
			{
				for (auto c : troop)
				{
					if (c->info == u.first)
					{
						if (auto ai = c->entity->get_component<cAI>(); ai)
						{
							if (!ai->target_node)
							{
								ai->target_node = target;
								n--;
								if (n == 0)
									break;
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

void TownInstance::add_attack_target(cNodePtr target)
{
	auto it = std::find_if(attack_list.begin(), attack_list.end(), [&](const auto& i) {
		return i == target;
	});
	if (it == attack_list.end())
	{
		attack_list.emplace_back(target);
	}
}

void TownInstance::remove_attack_target(cNodePtr target)
{
	for (auto it = attack_list.begin(); it != attack_list.end(); it++)
	{
		if (*it == target)
		{
			attack_list.erase(it);
			return;
		}
	}
	for (auto character : troop)
	{
		if (auto ai = character->entity->get_component<cAI>(); ai)
		{
			if (ai->target_node == target)
				ai->target_node = e->get_component<cNode>();
		}
	}
}

void TownInstance::update()
{
	auto num_construction = 1;
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
			add_building(c.building_info);
		num_construction--;
		if (num_construction == 0)
			break;
	}
	for (auto it = constructions.begin(); it != constructions.end();)
	{
		if (it->timer <= 0.f)
		{
			it = constructions.erase(it);
			constructions_changed_frame = frames;
		}
		else
			it++;
	}
	{
		auto t = delta_time / 60.f;
		{
			player->blood_fract += get_blood_production() * t;
			int i = floor(player->blood_fract);
			player->blood += i;
			player->blood_fract -= i;
		}
		{
			player->bones_fract += get_bones_production() * t;
			int i = floor(player->bones_fract);
			player->bones += i;
			player->bones_fract -= i;
		}
		{
			player->soul_sand_fract += get_soul_sand_production() * t;
			int i = floor(player->soul_sand_fract);
			player->soul_sand += i;
			player->soul_sand_fract -= i;
		}
	}
	for (auto& b : buildings)
	{
		auto num_training = b.number;
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
					if (auto character = add_character(t.unit_info, spawn_node->global_pos(), player->faction); character)
					{
						troop.push_back(character);
						character->entity->message_listeners.add([this, character](uint hash, void*, void*) {
							if (hash == "destroyed"_h)
							{
								std::erase_if(troop, [&](const auto& i) {
									return i == character;
								});
							}
						});
						if (auto ai = character->entity->get_component<cAI>(); ai)
						{
							ai->type = UnitLaneCreep;
							if (!attack_list.empty())
							{
								auto idx = linearRand(0, (int)attack_list.size() - 1);
								ai->target_node = attack_list[idx];
							}
						}
					}
				}
			}
			num_training--;
			if (num_training == 0)
				break;
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
	for (auto character : troop)
	{
		if (auto ai = character->entity->get_component<cAI>(); ai)
		{
			if (!ai->target_node)
			{
				if (!attack_list.empty())
				{
					auto idx = linearRand(0, (int)attack_list.size() - 1);
					ai->target_node = attack_list[idx];
				}
			}
		}
	}
}

void Player::init(EntityPtr e_town)
{
	{
		auto info = town_infos.find("Vampire");
		town.init(e_town, this, info);
	}
}

void Player::update()
{
	town.update();
}

Player player1;
Player player2;
