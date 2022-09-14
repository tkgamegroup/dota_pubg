#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewInventory : graphics::GuiView
{
	ViewInventory();

	bool on_open() override;
	void on_draw() override;
};

extern ViewInventory view_inventory;
