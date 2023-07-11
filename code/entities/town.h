#pragma once

#include "../head.h"
#include "../player.h"

extern std::vector<cTownPtr> towns;

// Reflect ctor
struct cTown : Component
{
	// Reflect requires
	cNodePtr node;

	TownInstance* ins;
	
	~cTown();
	void on_init() override;

	struct Create
	{
		virtual cTownPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
