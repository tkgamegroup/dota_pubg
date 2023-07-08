#pragma once

#include "head.h"
#include "presets.h"

struct Training
{
	const TrainingAction* action;
	const UnitInfo* unit_info;
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

struct Construction
{
	const ConstructionAction* action;
	const BuildingInfo* building_info;
	float duration;
	float timer;
	bool resources_costed;
};

struct TownInstance
{
	EntityPtr e = nullptr;
	Player* player;
	const TownInfo* info;

	uint hp_max;
	uint hp;

	std::vector<BuildingInstance> buildings;
	std::vector<Construction> constructions;
	uint constructions_changed_frame;

	cNodePtr spawn_node = nullptr;
	vec3 target_pos = vec3(0.f);

	void init(EntityPtr _e, Player* player, const TownInfo* info);
	uint get_blood_production() const;
	uint get_bones_production() const;
	uint get_soul_sand_production() const;
	void add_building(const BuildingInfo* info, int lv = 0);
	void add_construction(const ConstructionAction* action);
	void remove_construction(const ConstructionAction* action);
	void update();
};

// Reflect
struct Player
{
	// Reflect
	FactionFlags faction = FactionNone;

	// Reflect
	uint blood = 1000;
	float blood_fract = 0.f;
	// Reflect
	uint bones = 800;
	float bones_fract = 0.f;
	// Reflect
	uint soul_sand = 10;
	float soul_sand_fract = 0.f;

	// Reflect
	TownInstance town;

	void init(EntityPtr e_town);
	void update();
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
