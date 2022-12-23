#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewAbility : graphics::GuiView
{
	bool modal = false;

	ViewAbility();

	bool on_begin() override;
	void on_draw() override;
};

extern ViewAbility view_ability;
