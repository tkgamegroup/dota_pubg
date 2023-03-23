#pragma once

#include "../head.h"

extern std::vector<cShopPtr> shops;
extern std::vector<cShopPtr> dead_shops;
extern bool removing_dead_shops;

// Reflect ctor
struct cShop : Component
{
	// Reflect requires
	cNodePtr node;
	// Reflect auto_requires
	cObjectPtr object;

	int item_id = -1;
	uint item_num = 1;

	bool dead = false;

	~cShop();
	void on_init() override;
	void die();

	struct Create
	{
		virtual cShopPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
