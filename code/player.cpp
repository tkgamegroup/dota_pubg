#include <flame/graphics/material.h>
#include <flame/universe/components/mesh.h>

#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
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

void Player::init(EntityPtr e_town)
{
	blood = 1000;
	bones = 800;
	soul_sand = 10;

	town = e_town->get_component<cTown>();
	if (town)
		town->player = this;
}

void Player::update()
{
	if (town)
		town->update();
}

Player player1;
Player player2;
