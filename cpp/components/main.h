#pragma once

#include <flame/universe/component.h>

#include "../head.h"

// Reflect ctor
struct cMain : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float camera_length = 15.f;
	// Reflect
	float camera_angle = 45.f;

	~cMain();

	void on_active() override;
	void on_inactive() override;
	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
