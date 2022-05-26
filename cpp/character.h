#pragma once

#include "main.h"

enum State
{
	StateIdle,
	StateMove,
	StateBattle,
	StateBack
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

	cArmaturePtr armature;

	/// Reflect
	float radius = 0.6f;
	/// Reflect
	float height = 1.8f;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint atk = 10;
	/// Reflect
	float atk_distance = 1.5f;
	/// Reflect
	float atk_interval = 1.8f; // time
	/// Reflect
	float atk_precast = 0.5f; // 0-1, of the atk_interval
	/// Reflect
	uint faction = 0;
	/// Reflect
	uint ai_id = 0;

	bool dead = false;
	State state = StateIdle;
	Action action = ActionNone;
	vec3 started_pos;
	std::vector<cCharacterPtr> hate_list;
	cCharacterPtr target = nullptr;
	float search_timer = 0.f;
	float chase_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_timer = 0.f;

	void* gui_lis;

	~cCharacter();
	void on_init() override;
	std::vector<cCharacterPtr> find_enemies(float radius = 0.f);
	void enter_move_state(const vec3& pos);
	void enter_battle_state(const std::vector<cCharacterPtr>& enemies);
	void die();

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
