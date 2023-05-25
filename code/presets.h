#pragma once

#include "head.h"

struct UnitInfo
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;
};

// Reflect
struct UnitInfosPreset
{
	// Reflect
	std::vector<UnitInfo> infos;
};

struct BuildingInfo
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;
};

// Reflect
struct BuildingInfosPreset
{
	// Reflect
	std::vector<BuildingInfo> infos;
};
