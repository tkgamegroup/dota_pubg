#pragma once

#include "main.h"

struct BuffInstance
{
	uint id;
	float timer;
	float f0, f1, f2, f3;
};

struct Buff
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));

	void(*start)(BuffInstance* ins, cCharacterPtr character) = nullptr;
	void(*passive)(BuffInstance* ins, cCharacterPtr character) = nullptr;
	void(*continuous)(BuffInstance* ins, cCharacterPtr character) = nullptr;

	static int find(const std::string& name);
	static const Buff& get(uint id);
};

void init_buffs();
