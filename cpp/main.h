#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)

/// Reflect ctor
struct cMain : Component
{
	void on_active() override;
	void on_inactive() override;
	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
