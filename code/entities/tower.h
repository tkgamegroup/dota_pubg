#pragma once

#include "../head.h"
#include "../player.h"

// Reflect ctor
struct cTower : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	std::string info_name;

	Player* player = nullptr;
	const TowerInfo* info = nullptr;

	uint hp_max;
	uint hp;

	uint atk = 10;
	float atk_distance = 1.5f;
	float atk_interval = 2.f;
	const ProjectileInfo* atk_projectile = nullptr;

	Tracker target; // character

	float search_timer = 0.f;
	float attack_interval_timer = 0.f;

	~cTower();

	void start() override;
	void update() override;

	struct Create
	{
		virtual cTowerPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

extern std::vector<cTowerPtr> towers;
