#pragma once

#include "main.h"

/// Reflect ctor
struct cProjectile : Component
{
	/// Reflect requires
	cNodePtr node;

	float speed = 0.1f;

	cCharacterPtr target = nullptr;
	void set_target(cCharacterPtr character);

	std::function<void(bool)> callback;

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
