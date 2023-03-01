#pragma once

#include <flame/graphics/gui.h>

struct ViewSettings : graphics::GuiView
{
	ViewSettings();

	bool on_begin() override;
	void on_draw() override;
};

extern ViewSettings view_settings;
