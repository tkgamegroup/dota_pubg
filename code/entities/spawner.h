#pragma once

#include "../head.h"

// Reflect ctor
struct cSpawner : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	std::string character_name;
	// Reflect
	FactionFlags faction = FactionCreep;
	// Reflect
	float spawn_delay = 3.f;
	// Reflect
	float spawn_interval = 30.f;
	// Reflect
	uint spawn_number = 4;
	// Reflect
	UnitType unit_type = UnitCampCreep;
	// Reflect
	vec3 unit_target_pos = vec3(0.f);

	float spawn_timer = 0.f;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cSpawnerPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
