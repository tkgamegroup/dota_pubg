#pragma once

#include "../head.h"
#include "../player.h"

// Reflect ctor
struct cTown : Component
{
	// Reflect requires
	cNodePtr node;

	TownInstance* ins;

	struct Create
	{
		virtual cTownPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
