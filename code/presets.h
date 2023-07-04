#pragma once

#include "head.h"

struct UnitInfo
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	std::filesystem::path	prefab_name;
	uint					hp = 100;
	uint					atk = 10;
	float					atk_distance = 1.5f;
	float					atk_interval = 2.f;
	float					atk_time = 1.5f;
	float					atk_point = 1.f;
	uint					def = 0;
};

struct UnitAction
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;
	std::string				tooltip;

	VirtualUdt<Action>		click_action;
};

struct TrainingAction
{
	std::string				name;
	uint					time;
};

// Reflect
struct UnitInfosPreset
{
	// Reflect
	std::vector<UnitInfo> infos;
};

extern UnitInfosPreset unit_infos;

struct BuildingInfo
{
	std::string					name;
	std::filesystem::path		icon_name;
	uvec2						icon_tile_coord = uvec2(0);
	vec4						icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string					description;

	std::vector<TrainingAction> training_actions;
	std::vector<UnitAction>		actions;
};

// Reflect
struct BuildingInfosPreset
{
	// Reflect
	std::vector<BuildingInfo> infos;

	const BuildingInfo* find(std::string_view name) const;
};

extern BuildingInfosPreset building_infos;

void init_presets();
