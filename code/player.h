#pragma once

#include "head.h"
#include "presets.h"

// Reflect
struct Player
{
	// Reflect
	uint faction;
	uint character_id;

	// Reflect
	uint gold = 0;
	// Reflect
	uint wood = 0;
	// Reflect
	uint stone = 0;

	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cNavAgentPtr nav_agent = nullptr;
	cCharacterPtr character = nullptr;

	void init(EntityPtr e);
	std::vector<BuildingInfo> get_avaliable_building_infos() const;
	std::vector<UnitInfo> get_avaliable_unit_infos() const;
};
// Reflect
extern Player main_player;
