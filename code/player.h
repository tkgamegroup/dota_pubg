#pragma once

#include "head.h"
#include "presets.h"

struct BuildingInstance
{
	BuildingInfo* info;
	int lv = 0;
};

// Reflect
struct Player
{
	// Reflect
	FactionFlags faction = FactionNone;

	// Reflect
	uint gold = 0;
	// Reflect
	uint wood = 0;
	// Reflect
	uint stone = 0;
	// Reflect
	uint food = 0;

	// Reflect
	uint town_hp_max = 30;
	// Reflect
	uint town_hp = 30;

	EntityPtr e_town = nullptr;
	std::vector<UnitInfo*> formation;
	std::vector<cCharacterPtr> troop;

	std::vector<BuildingInstance> buildings;

	void init(EntityPtr _e_town);
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
