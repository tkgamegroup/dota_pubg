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
	std::vector<cCharacterPtr> troop;
	std::vector<cNodePtr> attack_list;
	uint attacks_changed_frame;

	void init(EntityPtr _e, Player* player, const TownInfo* info);
	uint get_blood_production() const;
	uint get_bones_production() const;
	uint get_soul_sand_production() const;
	void add_building(const BuildingInfo* info, int lv = 0);
	void add_construction(const ConstructionAction* action);
	void remove_construction(const ConstructionAction* action);
	void add_attack_target(cNodePtr target);
	void remove_attack_target(cNodePtr target);
	void update();
};

struct TowerInstance
{
	EntityPtr e = nullptr;
	Player* player;

	void init(EntityPtr _e, const TowerInfo* info);
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
