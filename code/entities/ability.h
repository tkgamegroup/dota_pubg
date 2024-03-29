#pragma once

#include "../head.h"

struct ActiveAbilityFunc
{ 
	virtual ~ActiveAbilityFunc() {}

	virtual void exec(cAbilityPtr ability, cCharacterPtr character, const vec3& location, cCharacterPtr target) = 0;
};

struct PassiveAbilityFunc
{
	virtual ~PassiveAbilityFunc() {}

	virtual void exec(cAbilityPtr ability, cCharacterPtr character) = 0;
};

// Reflect ctor
struct cAbility : Component
{
	// Reflect
	std::filesystem::path	icon_name;
	// Reflect
	uvec2					icon_tile_coord = uvec2(0);
	// Reflect
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	// Reflect
	std::string				description;

	// Reflect
	TargetTypeFlags			target_type = TargetNull;
	// Reflect
	uint					max_lv = 5;
	// Reflect
	float					cast_time = 0.f;
	// Reflect
	float					cast_point = 0.f;
	// Reflect
	float					channel_time = 0.f;
	// Reflect
	uint					mp = 0;
	// Reflect
	float					cd = 0;
	// Reflect
	float					distance = 0.f;
	// Reflect
	float					range = 0.f;
	// Reflect
	uint lv = 0;

	float cd_max = 0.f;
	float cd_timer = 0.f;

	// Reflect
	VirtualUdt<ActiveAbilityFunc>	active;
	// Reflect
	VirtualUdt<PassiveAbilityFunc>	passive;

	struct Create
	{
		virtual cAbilityPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

struct Talent
{
	std::string						name;
	std::vector<std::vector<uint>>	ablilities_list; // layers of ability hashes
};
