#pragma once

#include "main.h"

struct State
{
	enum Type
	{
		Idle,
		Battle
	}type;

	cCharacterPtr character = nullptr;

	virtual void update() = 0;
};

struct IdleState : State
{
	void update() override;
};

struct MoveState : State
{
	void update() override;
};

struct BattleState : State
{
	enum Action
	{
		Waiting,
		Chase,
		Attack
	}action;

	std::vector<cCharacterPtr> enemies;
	cCharacterPtr target = nullptr;
	vec3 start_pos;
	uint attack_counter;

	void update() override;
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
	Party party = LeftSide;
	/// Reflect
	uint radius = 6;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint atk = 10;
	/// Reflect
	uint atk_radius = 15;
	/// Reflect
	uint atk_frames = 30;
	/// Reflect
	uint disappear_frames = 30;

	/// Reflect
	bool passive = false;

	std::unique_ptr<State> state;
	bool dead = false;

	cCharacter();
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
