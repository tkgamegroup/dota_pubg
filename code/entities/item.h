#pragma once

#include "../head.h"

enum ItemType
{
	ItemItem,
	ItemConsumable
};

struct ActiveItemFunc
{
	virtual ~ActiveItemFunc() {}

	virtual void exec(cItemPtr item, cCharacterPtr character) = 0;
};

struct PassiveItemFunc
{
	virtual ~PassiveItemFunc() {}

	virtual void exec(cItemPtr item, cCharacterPtr character) = 0;
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

	// Reflect
	VirtualUdt<ActiveItemFunc>	active;
	// Reflect
	VirtualUdt<PassiveItemFunc>	passive;
};
