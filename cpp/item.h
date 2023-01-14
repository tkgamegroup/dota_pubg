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

	ParameterNames			parameter_names;
	ParameterPack				parameters;
	CommandList				active;
	CommandList				passive;

	static int find(const std::string& name);
	static const Item& get(uint id);
};

extern std::vector<Item> items;

void init_items();
