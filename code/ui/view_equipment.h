#pragma once

#include <flame/graphics/gui.h>

#include "../head.h"

struct ViewEquipment : graphics::GuiView
{
	ViewEquipment();

	bool on_begin() override;
	void on_draw() override;
};

extern ViewEquipment view_equipment;
