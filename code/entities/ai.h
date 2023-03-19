#pragma once

#include "../head.h"

// Reflect ctor
struct cCreepAI : Component
{
	// Reflect requires
	cCharacterPtr character;

	CreepType type = CreepCamp;
	vec3 start_pos;
	vec3 target_pos;
	float aggro_timer = 0.f;
	float flee_timer = 0.f;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCreepAIPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
