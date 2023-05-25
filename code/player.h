#pragma once

#include "head.h"
#include "presets.h"

// Reflect
struct Player
{
	// Reflect
	uint faction;

	// Reflect
	uint gold = 0;
	// Reflect
	uint wood = 0;
	// Reflect
	uint stone = 0;

	void init();
	std::vector<std::string> avaliable_building_infos;
	std::vector<std::string> avaliable_unit_infos;
};
// Reflect
extern Player main_player;
