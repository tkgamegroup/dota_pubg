#include "tower.h"

struct cTowerCreate : cTower::Create
{
	cTowerPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cTower;
	}
}cTower_create;
cTower::Create& cTower::create = cTower_create;
