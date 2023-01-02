#pragma once

#include "main.h"
#include "command.h"

struct AbilityInstance
{
	uint id;
	uint lv = 0;
	float cd_max = 0.f;
	float cd_timer = 0.f;
};

struct Ability
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	TargetType				target_type = TargetNull;
	uint					max_lv = 5;
	float					cast_time = 0.f;
	float					channel_time = 0.f;
	uint					mp = 0;
	float					cd = 0.f;
	float					cast_distance = 0.f;
	float					cast_range = 0.f;
	float					cast_angle = 0.f;

	ParameterNames			parameter_names;
	ParameterPack			parameters;
	CommandList				active;
	CommandList				passive;

	static int find(const std::string& name);
	static const Ability& get(uint id);
};

struct Talent
{
	uint							id;
	std::string						name;
	std::vector<std::vector<uint>>	ablilities_list; // layers of ability ids

	static int find(const std::string& name);
	static const Talent& get(uint id);
};

void init_abilities();
