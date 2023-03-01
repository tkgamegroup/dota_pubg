#pragma once

#include <flame/universe/universe.h>

#include "head.h"

struct MainTerrain
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cTerrainPtr hf_terrain = nullptr; // height field terrain
	cVolumePtr mc_terrain = nullptr; // marching cubes terrain
	vec3 extent;

	std::vector<vec3> site_positions;
	std::vector<std::pair<float, int>> site_centrality;

	void init(EntityPtr e);
	vec3 get_coord(const vec2& uv);
	vec3 get_coord(const vec3& pos);
	vec3 get_coord_by_centrality(int i);
};
extern MainTerrain main_terrain;

void init_map();
