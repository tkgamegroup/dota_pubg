#pragma once

#include "main.h"

struct Buff
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	static int find(const std::string& name);
	static const Buff& get(uint id);
};

void load_buffs();
