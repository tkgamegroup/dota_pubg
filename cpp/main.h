#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)

/// Reflect ctor
struct cMain : Component
{
	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
