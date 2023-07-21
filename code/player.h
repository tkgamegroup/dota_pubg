#pragma once

#include "head.h"
#include "presets.h"

struct Training
{
	const TrainingAction* action;
	const CharacterInfo* unit_info;
	float duration;
	float timer;
	int number; // -1 means infinite
	bool resources_costed;
};

struct Player;

struct BuildingInstance
{
	Player* player;
	const BuildingInfo* info;
	uint number = 0;

	std::vector<Training> trainings;
	uint trainings_changed_frame;

	void add_training(const TrainingAction* action, int number, bool new_training = false);
	void remove_training(uint idx);
};

struct Construction
{
	const ConstructionAction* action;
	const BuildingInfo* building_info;
	float duration;
	float timer;
	bool resources_costed;
};

struct AttackAction
{
	cNodePtr target;
	std::vector<std::pair<CharacterInfo*, uint>> formation;
};

// Reflect
struct Player
{
	// Reflect
	FactionFlags faction = FactionNone;

	// Reflect
	uint blood = 1;
	float blood_fract = 0.f;
	// Reflect
	uint bones = 1;
	float bones_fract = 0.f;
	// Reflect
	uint soul_sand = 1;
	float soul_sand_fract = 0.f;

	// Reflect
	cTownPtr town;

	void init(EntityPtr e_town);
	void update();
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
