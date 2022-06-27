#pragma once

#include "main.h"

enum State
{
	StateIdle,
	StateMove,
	StateMoveAttack,
	StateBattle
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
	float radius = 0.3f;
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
	float atk_precast = 0.43f; // 0-1, of the atk_interval
	/// Reflect
	uint faction = 0;
	/// Reflect
	uint ai_id = 0;

	bool dead = false;
	State state = StateIdle;
	State next_state = StateIdle;
	Action action = ActionNone;
	vec3 started_pos;
	vec3 move_target;
	cCharacterPtr target = nullptr;
	float search_timer = 0.f;
	float chase_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_timer = 0.f;

	~cCharacter();
	void on_init() override;
	void on_active() override;
	void on_inactive() override;
	std::vector<cCharacterPtr> find_enemies(float radius = 0.f);
	void change_target(cCharacterPtr character);
	void enter_move_state(const vec3& pos);
	void enter_move_attack_state(const vec3& pos);
	void enter_battle_state(cCharacterPtr target);
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
