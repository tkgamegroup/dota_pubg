#pragma once

#include "head.h"
#include "presets.h"

inline const auto FORMATION_CX = 4;
inline const auto FORMATION_CY = 4;
inline const auto FORMATION_GAP = 2.f;

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

	vec3 troop_target_location = vec3(0.f);

	std::vector<UnitInfo*> formation;
	EntityPtr e_town = nullptr;
	EntityPtr e_formation_grid = nullptr;
	std::vector<cCharacterPtr> troop;

	std::vector<BuildingInfo*> avaliable_building_infos;
	std::vector<UnitInfo*> avaliable_unit_infos;

	void init(EntityPtr _e_town);
	void set_formation(uint index, UnitInfo* unit_info);
	void spawn_troop();
	void remove_troop();
};

// Reflect
extern Player player1;
// Reflect
extern Player player2;
