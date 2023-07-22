#pragma once
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

#include "../head.h"

extern EntityPtr ui;

void init_ui();
void deinit_ui();
void update_ui();
cvec4 get_player_color(FactionFlags faction);
void show_tooltip(const vec2& pos, const std::wstring& text);
void show_tooltip(const vec2& pos, const std::wstring& text, uint blood, uint bones, uint soul_sand);
void close_tooltip();
void add_ui_message(const std::wstring& text, const cvec4& color = cvec4(255));
