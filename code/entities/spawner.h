#pragma once

#include "../head.h"

using namespace flame;

// Reflect ctor
struct cSpawner : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	uint preset_id = 0;
	// Reflect
	uint faction = FactionCreep;
	// Reflect
	float spawn_interval = 30.f;
	// Reflect
	uint spawn_number = 4;

	float spawn_timer = 0.f;

	void update() override;

	struct Create
	{
		virtual cSpawner* operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
