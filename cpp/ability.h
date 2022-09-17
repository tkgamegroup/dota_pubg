#pragma once

#include "main.h"

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

	bool(*cast_check)(cCharacterPtr) = nullptr;
	void(*active)(cCharacterPtr) = nullptr;
	void(*active_l)(cCharacterPtr, const vec3&) = nullptr;
	void(*active_t)(cCharacterPtr, cCharacterPtr) = nullptr;
	void(*channel_l)(cCharacterPtr, const vec3&) = nullptr;
	void(*channel_t)(cCharacterPtr, cCharacterPtr) = nullptr;
	void(*show)() = nullptr;

	static int find(const std::string& name);
	static const Ability& get(uint id);
};
