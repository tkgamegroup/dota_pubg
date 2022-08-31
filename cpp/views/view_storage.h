#pragma once

#include "../main.h"

#include <flame/graphics/gui.h>

struct ViewStorage : graphics::GuiView
{
	void on_draw() override;
};
