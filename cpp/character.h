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
	uint radius = 6;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint atk = 10;
	/// Reflect
	uint atk_distance = 15;
	/// Reflect
	uint atk_speed = 30;
	/// Reflect
	uint ai_id = 0;

	bool dead = false;
	State state = StateIdle;
	Action action = ActionNone;
	vec3 started_pos;
	std::vector<cCharacterPtr> hate_list;
	uint attack_tick = 0;

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
