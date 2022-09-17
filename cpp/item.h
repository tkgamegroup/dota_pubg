#pragma once

#include "main.h"

enum ItemType
{
	ItemItem,
	ItemEquipment,
	ItemConsumable
};

enum EquipPart
{
	EquipHead,
	EquipNeck,
	EquipShoulder,
	EquipChest,
	EquipBack,
	EquipHand,
	EquipLeg,
	EquipFoot,
	EquipFinger0,
	EquipFinger1,
	EquipWeapon0,
	EquipWeapon1,

	EquipPart_Count
};

struct WeaponInfo
{
	DamageType atk_type;
	uint atk;
};

struct EquipmentInfo
{
	EquipPart part;

	WeaponInfo& (*weapon_info)() = nullptr;
};

struct Item
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	ItemType				type = ItemItem;

	EquipmentInfo&(*equipment_info)() = nullptr;
	void(*active)(cCharacterPtr) = nullptr;
	void(*passive)(cCharacterPtr) = nullptr;
	void(*show)() = nullptr;

	static int find(const std::string& name);
	static const Item& get(uint id);
};
