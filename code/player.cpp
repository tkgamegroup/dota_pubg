#include "player.h"
#include "presets.h"
#include "entities/character.h"

void Player::init()
{
	if (!unit_infos.infos.empty())
		avaliable_unit_infos.push_back(unit_infos.infos.front().name);
}

Player main_player;
