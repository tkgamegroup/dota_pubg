#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewSettings : graphics::GuiView
{
	ViewSettings();

	bool on_open() override;
	void on_draw() override;
};

extern ViewSettings view_settings;
