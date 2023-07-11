#pragma once

#include "head.h"

// Reflect any ctor
struct CharacterInfo
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	std::filesystem::path	prefab_name;
	float					nav_radius = 0.2f;
	float					nav_height = 2.f;
	float					nav_speed = 5.f;
	uint					hp_max = 100;
	uint					mp_max = 100;
	DamageType				atk_type = PhysicalDamage;
	uint					atk = 10;
	float					atk_distance = 1.5f;
	float					atk_interval = 2.f; // the time between two attacks
	float					atk_time = 1.5f; // the time of attack animation
	float					atk_point = 1.f; // the time that attack happens
	std::string				atk_projectile_name;
	uint					phy_def = 0;
	uint					mag_def = 0;
	uint					hp_reg = 0;
	uint					mp_reg = 0;
	uint					mov_sp = 100;
	uint					atk_sp = 100;
};

struct ProjectileInfo
{
	std::string				name;

	std::filesystem::path	prefab_name;
};

struct EffectInfo
{
	std::string				name;

	std::filesystem::path	prefab_name;
};

struct ResourceProduction
{
	ResourceType			type;
	uint					amount; // per minute
};

struct TrainingAction
{
	std::string				name;
	uint					cost_blood;
	uint					cost_bones;
	uint					cost_soul_sand;
	float					duration;
};

struct ConstructionAction
{
	std::string				name;
	uint					cost_blood;
	uint					cost_bones;
	uint					cost_soul_sand;
	float					duration;

};

// Reflect
struct CharacterInfosPreset
{
	// Reflect
	std::vector<CharacterInfo> infos;

	const CharacterInfo* find(std::string_view name) const;
};

extern CharacterInfosPreset character_infos;

// Reflect
struct ProjectileInfosPreset
{
	// Reflect
	std::vector<ProjectileInfo> infos;

	const ProjectileInfo* find(std::string_view name) const;
};

extern ProjectileInfosPreset projectile_infos;

// Reflect
struct EffectInfosPreset
{
	// Reflect
	std::vector<EffectInfo> infos;

	const EffectInfo* find(std::string_view name) const;
};

extern EffectInfosPreset effect_infos;

struct BuildingInfo
{
	std::string						name;
	std::filesystem::path			icon_name;
	uvec2							icon_tile_coord = uvec2(0);
	vec4							icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string						description;

	std::vector<ResourceProduction> resource_production;
	std::vector<TrainingAction>		training_actions;
};

// Reflect
struct BuildingInfosPreset
{
	// Reflect
	std::vector<BuildingInfo> infos;

	const BuildingInfo* find(std::string_view name) const;
};

extern BuildingInfosPreset building_infos;

struct TownInfo
{
	std::string							name;
	std::filesystem::path				icon_name;
	uvec2								icon_tile_coord = uvec2(0);
	vec4								icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string							description;

	uint								hp_max;
	std::vector<ConstructionAction>		construction_actions;
};

// Reflect
struct TownInfosPreset
{
	// Reflect
	std::vector<TownInfo> infos;

	const TownInfo* find(std::string_view name) const;
};

extern TownInfosPreset town_infos;

void init_presets();
