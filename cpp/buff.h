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
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	void(*start)(cCharacterPtr, BuffInstance*) = nullptr;
	void(*passive)(cCharacterPtr, BuffInstance*) = nullptr;
	void(*continuous)(cCharacterPtr, BuffInstance*) = nullptr;

	static int find(const std::string& name);
	static const Buff& get(uint id);
};

void load_buffs();
