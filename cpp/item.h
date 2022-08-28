#pragma once

#include "main.h"

enum ItemType
{
	ItemItem,
	ItemEquipment,
	ItemConsumable
};

struct Item
{
	uint					id;
	std::string				name;
	ItemType				type = ItemItem;
	std::filesystem::path	icon_name;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	void(*active)(cCharacterPtr) = nullptr;
	void(*passive)(cCharacterPtr) = nullptr;
	void(*show)() = nullptr;

	static int find(const std::string& name);
	static const Item& get(uint id);
};

