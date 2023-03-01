#pragma once

#include <flame/universe/component.h>

#include "../head.h"

extern std::vector<cChestPtr> chests;
extern std::vector<cChestPtr> dead_chests;
extern bool removing_dead_chests;

// Reflect ctor
struct cChest : Component
{
	// Reflect requires
	cNodePtr node;
	// Reflect auto_requires
	cObjectPtr object;

	int item_id = -1;
	uint item_num = 1;

	bool dead = false;

	~cChest();
	void on_init() override;
	void die();

	struct Create
	{
		virtual cChestPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
