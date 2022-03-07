#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)
FLAME_TYPE(cPlayer)
FLAME_TYPE(cCharacter)

/// Reflect
enum Party
{
	LeftSide = 1 << 1,
	RightSide = 1 << 2,
	MiddleSide = 1 << 3
};

struct Ref
{
	EntityPtr e;
	bool invalid = false;
	uint n = 1;
	void* lis;
};

struct RefManager
{
	std::map<EntityPtr, Ref> refs;

	Ref* get(EntityPtr e);
	void release(Ref* ref);

	static RefManager& instance;
};

/// Reflect ctor
struct cMain : Component
{
	/// Reflect
	float camera_length = 10.f;
	/// Reflect
	float camera_angle = 45.f;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
