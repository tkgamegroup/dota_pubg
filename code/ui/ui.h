#pragma once
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>

#include "../head.h"

struct UI_building_window
{
	EntityPtr window = nullptr;
	EntityPtr building_area = nullptr;
	EntityPtr empty_case = nullptr;
	EntityPtr built_up_case = nullptr;

	void init();
	void open();
	void select_building_area(uint index);
};
extern UI_building_window ui_building_window;

struct UI_troop_window
{
	EntityPtr window = nullptr;
	EntityPtr formation_area = nullptr;
	EntityPtr unit_list = nullptr;

	void init();
	void open();
	void select_formation_slot(uint index);
	void select_unit_slot(int index);
};
extern UI_troop_window ui_troop_window;

extern EntityPtr ui;

void init_ui();
void deinit_ui();
void update_ui();
