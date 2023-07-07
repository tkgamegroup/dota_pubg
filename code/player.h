#pragma once

#include "head.h"
#include "presets.h"

struct Training
{
	const TrainingAction* action;
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
	int lv = 0;

	std::vector<Training> trainings;
	uint trainings_changed_frame;

	void add_training(const TrainingAction* action, int number);
	void remove_training(const TrainingAction* action);
};

// Reflect
struct Player
{
	// Reflect
	FactionFlags faction = FactionNone;

	// Reflect
	uint blood = 1000;
	// Reflect
	uint bones = 800;
	// Reflect
	uint soul_sand = 10;

	// Reflect
	uint town_hp_max = 30;
	// Reflect
	uint town_hp = 30;

	EntityPtr e_town = nullptr;
	cNodePtr spawn_node = nullptr;
	vec3 target_pos = vec3(0.f);

	std::vector<BuildingInstance> buildings;

	void init(EntityPtr _e_town);
	void add_building(const BuildingInfo* info, int lv = 0);
	void update();
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
