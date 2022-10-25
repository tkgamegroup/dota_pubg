#pragma once

#include "main.h"

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
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	ItemType				type = ItemItem;

	void(*active)(cCharacterPtr) = nullptr;
	void(*passive)(cCharacterPtr) = nullptr;
	void(*show)() = nullptr;

	static int find(const std::string& name);
	static const Item& get(uint id);
};
