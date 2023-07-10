#pragma once

#include "../head.h"

// Reflect ctor
struct cTower : Component
{
	// Reflect requires
	cNodePtr node;

	struct Create
	{
		virtual cTowerPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
