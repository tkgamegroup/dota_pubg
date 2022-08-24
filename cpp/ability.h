#pragma once

#include "main.h"

struct Ability
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	TargetType target_type = TargetNull;
	uint mana = 0;

	void(*active)(cCharacterPtr, cCharacterPtr) = nullptr;

	static const Ability& get(uint id);
};

void load_abilities();
