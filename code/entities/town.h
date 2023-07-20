#pragma once

#include "../head.h"
#include "../player.h"

// Reflect ctor
struct cTown : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	std::string info_name;

	Player* player;
	const TownInfo* info = nullptr;

	uint hp_max;
	uint hp;

	std::vector<BuildingInstance> buildings;
	std::vector<Construction> constructions;
	uint constructions_changed_frame = 0;

	cNodePtr spawn_node = nullptr;
	std::vector<cCharacterPtr> troop;
	std::vector<cNodePtr> attack_list;
	uint attacks_changed_frame = 0;
	
	~cTown();

	uint get_blood_production() const;
	uint get_bones_production() const;
	uint get_soul_sand_production() const;
	void add_building(const BuildingInfo* info);
	void add_construction(const ConstructionAction* action);
	void remove_construction(const ConstructionAction* action);
	bool send_troop(cNodePtr target, const std::vector<std::pair<const CharacterInfo*, uint>>& formation);
	void add_attack_target(cNodePtr target);
	void remove_attack_target(cNodePtr target);

	void start() override;
	void update() override;

	struct Create
	{
		virtual cTownPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

extern std::vector<cTownPtr> towns;
