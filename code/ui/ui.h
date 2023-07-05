#pragma once
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

#include "../head.h"

extern EntityPtr ui;

void init_ui();
void deinit_ui();
void update_ui();
void show_tooltip(const vec2& pos, const std::wstring& text);
void close_tooltip();
