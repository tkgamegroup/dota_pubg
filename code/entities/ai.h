#pragma once

#include "../head.h"

// Reflect ctor
struct cCreepAI : Component
{
	enum Type
	{
		TypeCamp,
		TypeLane
	};

	// Reflect requires
	cCharacterPtr character;

	Type type = TypeCamp;
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
