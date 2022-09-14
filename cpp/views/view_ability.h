#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewAbility : graphics::GuiView
{
	ViewAbility();

	bool on_open() override;
	void on_draw() override;
};

extern ViewAbility view_ability;
