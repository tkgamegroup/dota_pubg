#pragma once

#include "main.h"
#include "command.h"

struct BuffInstance
{
	uint id;
	uint lv = 0;
	float timer;
	float t0;
	float interval;
	float duration;
};

struct Buff
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));

	float					interval;

	std::string				description;

	ParameterNames			parameter_names;
	Parameters				parameters;
	CommandList				passive;
	CommandList				continuous;

	static int find(const std::string& name);
	static const Buff& get(uint id);
};

void init_buffs();
