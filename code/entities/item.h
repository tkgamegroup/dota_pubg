#pragma once

#include "../head.h"
#include "../command.h"

enum ItemType
{
	ItemItem,
	ItemConsumable
};

// Reflect ctor
struct cItem : Component
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	ItemType				type = ItemItem;
	uint					num = 1;

	ParameterNames			parameter_names;
	ParameterPack			parameters;
	CommandList				active;
	CommandList				passive;
};
