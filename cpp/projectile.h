#pragma once

#include "main.h"

/// Reflect ctor
struct cProjectile : Component
{
	/// Reflect requires
	cNodePtr node;

	float speed = 0.1f;

	bool			use_target = true;
	vec3			location;
	cCharacterPtr	target = nullptr;
	void set_target(cCharacterPtr character);

	float collide_radius = 0.f;
	uint collide_faction = 0;

	std::function<void(cProjectilePtr pt)>				on_update;
	std::function<void(cCharacterPtr c)>				on_collide;
	std::function<void(const vec3& l, cCharacterPtr t)> on_end;

	void die();

	void start() override;
	void update() override;

	struct Create
	{
		virtual cProjectilePtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
 