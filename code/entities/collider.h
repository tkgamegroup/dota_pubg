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
	FactionFlags faction = FactionNone;
	// Reflect
	float collide_delay = 0.f;

	float off = 0.f;
	float r0, r1;
	vec3 c;
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
