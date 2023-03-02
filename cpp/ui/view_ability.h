#pragma once

#include <flame/graphics/gui.h>

#include "../head.h"

struct ViewAbility : graphics::GuiView
{
	ViewAbility();

	bool on_begin() override;
	void on_draw() override;
};

extern ViewAbility view_ability;
