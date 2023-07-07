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
	uint					cost_blood;
	uint					cost_bones;
	uint					cost_soul_sand;
	float					duration;
};

// Reflect
struct UnitInfosPreset
{
	// Reflect
	std::vector<UnitInfo> infos;

	const UnitInfo* find(std::string_view name) const;
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
