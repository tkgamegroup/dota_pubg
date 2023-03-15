#pragma once

#include "../head.h"
#include "../command.h"

// Reflect ctor
struct cBuff : Component
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	uint lv = 0;
	float timer;
	float t0;
	float interval;
	float duration;

	ParameterNames			parameter_names;
	ParameterPack			parameters;
	CommandList				passive;
	CommandList				continuous;
};
