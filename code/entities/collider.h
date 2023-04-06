#pragma once

#include "../head.h"
#include "../command.h"

// Reflect ctor
struct cCircleCollider : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float radius = 5.f;
	// Reflect
	FactionFlags faction = FactionNone;

	std::vector<Tracker> last_collidings;
	Listeners<void(cCharacterPtr character, bool enter_or_exit)> callbacks;

	void update() override;

	struct Create
	{
		virtual cCircleColliderPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

// Reflect ctor
struct cSectorCollider : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float inner_radius = 0.f;
	// Reflect
	float outer_radius = 0.f;
	// Reflect
	float length = 2.f;
	// Reflect
	float angle = 0.f;
	// Reflect
	float speed = 0.f;
	// Reflect
	FactionFlags faction = FactionNone;
	// Reflect
	float delay = 0.f;
	// Reflect
	float duration = 1.f;

	float off = 0.f;
	float r0, r1;
	float yaw_angle;
	float t;

	std::vector<Tracker> last_collidings;
	Listeners<void(cCharacterPtr character, bool enter_or_exit)> callbacks;

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

extern bool enable_collider_debugging;
