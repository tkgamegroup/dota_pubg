#pragma once

#include "main.h"

/// Reflect ctor
struct cPlayer : Component
{
	/// Reflect requires
	cCharacterPtr character;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cPlayerPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
