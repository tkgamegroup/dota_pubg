#pragma once

#include "main.h"
#include "command.h"

// Reflect ctor
struct cSectorCollider : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float radius = 0.f;
	// Reflect
	float start_radius = 0.f;
	// Reflect
	float window_length = 0.f;
	// Reflect
	float central_angle = 0.f;
	// Reflect
	float direction_angle = 0.f;
	// Reflect
	float speed = 0.f;
	// Reflect
	uint faction = 0;
	// Reflect
	vec2 collide_time = vec2(0.f, 1.f);

	float off = 0.f;
	float r0, r1;
	vec3 c;
	float t;
	uint rnd;

	CommandList callback;
	cCharacterPtr host = nullptr;

	void on_active() override;
	void on_inactive() override;
	void update() override;

	struct Create
	{
		virtual cSectorColliderPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
