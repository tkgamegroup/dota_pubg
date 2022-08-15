#pragma once

#include "main.h"

struct Item
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	vec4					icon_uvs;
	graphics::ImagePtr		icon_image = nullptr;

	uint add_mov_sp = 0;
	uint add_atk_sp = 0;

	static Item& get(uint id);
};

void load_items();
