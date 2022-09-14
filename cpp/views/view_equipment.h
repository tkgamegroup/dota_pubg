#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewEquipment : graphics::GuiView
{
	ViewEquipment();

	bool on_open() override;
	void on_draw() override;
};

extern ViewEquipment view_equipment;
