#pragma once

#include "main.h"
#include "command.h"

enum ItemType
{
	ItemItem,
	ItemConsumable
};

struct ItemInstance
{
	uint id;
	uint num = 1;
};

struct Item
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));

	ItemType				type = ItemItem;

	std::string				description;

	std::vector<std::pair<std::string, uint>> 
							parameter_names;
	Parameters				parameters;
	std::vector<Command>	active;
	std::vector<Command>	passive;

	static int find(const std::string& name);
	static const Item& get(uint id);
};

void init_items();
