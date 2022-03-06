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

struct BattleState : State
{
	enum Action
	{
		Chase,
		Attack
	}action;

	std::vector<cCharacterPtr> enemies;
	cCharacterPtr target = nullptr;

	void update() override;
};

/// Reflect ctor
struct cCharacter : Component
{
	/// Reflect requires
	cNodePtr node;
	/// Reflect requires
	cNavAgentPtr nav;

	/// Reflect
	Party party = LeftSide;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint atk = 10;

	std::unique_ptr<State> state;

	cCharacter();
	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
