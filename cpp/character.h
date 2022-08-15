#pragma once

#include "main.h"

enum Command
{
	CommandIdle,
	CommandMoveTo,
	CommandAttackTarget,
	CommandAttackLocation
};

enum Action
{
	ActionNone,
	ActionMove,
	ActionAttack
};

/// Reflect ctor
struct cCharacter : Component
{
	/// Reflect requires
	cNodePtr node;
	/// Reflect requires
	cNavAgentPtr nav_agent;

	cArmaturePtr armature = nullptr;

	/// Reflect
	uint lv = 1;
	/// Reflect
	uint exp = 0;
	/// Reflect
	uint exp_max = 0;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint mp = 100;
	/// Reflect
	uint mp_max = 100;
	/// Reflect
	uint STR = 10;
	/// Reflect
	uint AGI = 10;
	/// Reflect
	uint INT = 10;
	/// Reflect
	uint atk = 10;
	/// Reflect
	float atk_distance = 1.5f;
	/// Reflect
	float atk_interval = 2.f; // time
	/// Reflect
	float atk_precast = 0.5f; // 0-1, of the atk_interval
	/// Reflect
	std::filesystem::path atk_projectile_name;
	/// Reflect
	void set_atk_projectile_name(const std::filesystem::path& name);
	/// Reflect
	uint armor = 0;
	/// Reflect
	uint mov_sp = 100;
	/// Reflect
	uint atk_sp = 100;
	/// Reflect
	uint faction = 0;
	/// Reflect
	uint ai_id = 0;

	int inventory[6] = { -1 };

	bool dead = false;
	bool stats_dirty = true;
	Command command;
	Action action = ActionNone;
	vec3 move_location;
	cCharacterPtr target = nullptr;
	float move_speed = 1.f;
	float attack_speed = 1.f;
	float search_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_timer = 0.f;
	EntityPtr atk_projectile = nullptr;

	~cCharacter();
	void on_init() override;

	std::vector<cCharacterPtr> find_enemies(float radius = 0.f, bool ignore_timer = true, bool sort = false);

	void set_target(cCharacterPtr character);
	void inflict_damage(cCharacterPtr target, uint value);
	bool take_damage(uint value); // return true if the damage causes the character die
	void manipulate_item(int idx0, int idx1, int item_id);
	void level_up();
	void die();

	void cmd_move_to(const vec3& pos);
	void cmd_attack_target(cCharacterPtr character);
	void cmd_attack_location(const vec3& pos);

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
