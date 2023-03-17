#pragma once

#include "head.h"

extern vec3				hovering_pos;
extern cCharacterPtr	hovering_character;
extern cChestPtr		hovering_chest;
extern bool				hovering_terrain;

void update_control();
void command_character_idle(cCharacterPtr character);
void command_character_hold(cCharacterPtr character);
void command_character_moveto(cCharacterPtr character, const vec3& pos);
void command_character_attack_target(cCharacterPtr character, cCharacterPtr target);
void command_character_attack_location(cCharacterPtr character, const vec3& pos);
void command_character_pickup(cCharacterPtr character, cChestPtr chest);