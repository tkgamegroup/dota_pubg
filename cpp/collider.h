#pragma once

#include "main.h"

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
	float length = 0.f;
	// Reflect
	float angle = 0.f;
	// Reflect
	float dir = 0.f;
	// Reflect
	float speed = 0.f;
	// Reflect
	uint faction = 0;

	float off = 0.f;

	Listeners<void(const std::vector<cCharacterPtr>& characters)> callbacks;

	void on_active() override;
	void on_inactive() override;
	void start() override;
	void update() override;

	struct Create
	{
		virtual cSectorColliderPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
