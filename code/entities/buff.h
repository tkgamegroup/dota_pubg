#pragma once

#include "../head.h"

struct PassiveBuffFunc
{
	virtual ~PassiveBuffFunc() {}

	virtual void exec(cBuffPtr buff, cCharacterPtr character) = 0;
};

struct ContinuousBuffFunc
{
	virtual ~ContinuousBuffFunc() {}

	virtual void exec(cBuffPtr buff, cCharacterPtr character) = 0;
};

// Reflect ctor
struct cBuff : Component
{
	uint					id;
	std::string				name;
	std::filesystem::path	icon_name;
	uvec2					icon_tile_coord = uvec2(0);
	graphics::ImagePtr		icon_image = nullptr;
	vec4					icon_uvs = vec4(vec2(0.f), vec2(1.f));
	std::string				description;

	uint lv = 0;
	float timer;
	float t0;
	float interval;
	float duration;

	// Reflect
	VirtualUdt<PassiveBuffFunc>	passive;
	// Reflect
	VirtualUdt<ContinuousBuffFunc>	continuous;
};
