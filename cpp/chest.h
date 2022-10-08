#pragma once

#include "main.h"

/// Reflect ctor
struct cChest : Component
{
	/// Reflect requires
	cNodePtr node;

	std::string guid;

	int item_id = -1;
	uint item_num = 1;

	void on_init() override;

	struct Create
	{
		virtual cChestPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
