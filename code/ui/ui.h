#pragma once
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

#include "../head.h"

extern EntityPtr ui_ability_slots[4];
extern cElementPtr ui_hp_bar;
extern cElementPtr ui_mp_bar;
extern cTextPtr ui_hp_text;
extern cTextPtr ui_mp_text;

void init_ui();
void deinit_ui();
void update_ui();
