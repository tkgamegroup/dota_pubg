#pragma once

#include "head.h"

extern vec3				hovering_pos;
extern cNodePtr			hovering_node;
extern cCharacterPtr	hovering_character;
extern cChestPtr		hovering_chest;
extern bool				hovering_terrain;
extern cNodePtr			hovering_town;

extern Tracker					selected_target;
extern Listeners<void()>		select_callbacks;
extern TargetTypeFlags			select_mode;
extern float					select_distance;
extern float					select_range;

void init_control();
void update_control();
void command_character_idle(cCharacterPtr character);
void command_character_hold(cCharacterPtr character);
void command_character_moveto(cCharacterPtr character, const vec3& pos);
void command_character_attack_target(cCharacterPtr character, cCharacterPtr target);
void command_character_attack_location(cCharacterPtr character, const vec3& pos);
void command_character_pickup(cCharacterPtr character, cChestPtr chest);
