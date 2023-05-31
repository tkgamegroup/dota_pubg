#pragma once

#include "head.h"
#include "presets.h"

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
	vec3 town_pos = vec3(0.f);
	// Reflect
	vec3 troop_spawn_off = vec3(0.f);
	// Reflect
	vec3 troop_target_pos = vec3(0.f);

	std::vector<UnitInfo*> formation;
	std::vector<cCharacterPtr> troop;

	std::vector<BuildingInfo*> avaliable_building_infos;
	std::vector<UnitInfo*> avaliable_unit_infos;

	void init(EntityPtr e_town);
	void spawn_troop();
	void remove_troop();
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
