#pragma once

#include "main.h"

struct AbilityInstance
{
	uint id;
	uint lv = 0;
	float cd_max = 0.f;
	float cd_timer = 0.f;
};

struct Ability
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	graphics::ImagePtr		icon_image = nullptr;

	TargetType target_type = TargetNull;
	float cast_time = 0.f;
	float channel_time = 0.f;
	uint mp = 0;
	float cd = 0.f;
	float distance = 0.f;
	float range = 0.f;
	float angle = 0.f;

	bool(*cast_check)(uint lv, cCharacterPtr caster) = nullptr;
	void(*active)(uint lv, cCharacterPtr caster) = nullptr;
	void(*active_l)(uint lv, cCharacterPtr caster, const vec3&) = nullptr;
	void(*active_t)(uint lv, cCharacterPtr caster, cCharacterPtr) = nullptr;
	void(*channel_l)(uint lv, cCharacterPtr caster, const vec3&) = nullptr;
	void(*channel_t)(uint lv, cCharacterPtr caster, cCharacterPtr) = nullptr;
	void(*passive)(uint lv, cCharacterPtr caster) = nullptr;
	void(*show)(uint lv) = nullptr;

	static int find(const std::string& name);
	static const Ability& get(uint id);
};
