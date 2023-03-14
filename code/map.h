#pragma once

#include "head.h"

void init_map(EntityPtr e);
vec3 get_map_coord(const vec2& uv);
vec3 get_map_coord(const vec3& pos);
vec3 get_coord_by_centrality(int i);
void init_vision();
void deinit_vision();
bool get_vision(uint faction, const vec3& coord);
void update_vision();
