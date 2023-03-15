#pragma once

#include "../head.h"
#include "../command.h"

// Reflect ctor
struct cAbility : Component
{
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	TargetType				target_type = TargetNull;
	uint					max_lv = 5;
	float					cast_time = 0.f;
	float					channel_time = 0.f;
	std::vector<uint>		mp;
	std::vector<float>		cd;
	std::vector<float>		distance;
	std::vector<float>		range;
	float					angle = 0.f;
	float					start_radius = 0.f;

	uint lv = 0;
	float cd_max = 0.f;
	float cd_timer = 0.f;

	inline uint get_mp(uint lv) const
	{
		if (mp.empty() || lv == 0) return 0;
		if (mp.size() == 1) return mp[0];
		return mp[lv - 1];
	}

	inline float get_cd(uint lv) const
	{
		if (cd.empty() || lv == 0) return 0;
		if (cd.size() == 1) return cd[0];
		return cd[lv - 1];
	}

	inline float get_distance(uint lv) const
	{
		if (distance.empty() || lv == 0) return 0;
		if (distance.size() == 1) return distance[0];
		return distance[lv - 1];
	}

	inline float get_range(uint lv) const
	{
		if (range.empty() || lv == 0) return 0;
		if (range.size() == 1) return range[0];
		return range[lv - 1];
	}

	ParameterNames			parameter_names;
	ParameterPack			parameters;
	CommandList				active;
	CommandList				passive;

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
