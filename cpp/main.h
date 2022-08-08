#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cSpwaner)
FLAME_TYPE(cProjectile)

const auto CharacterTag = 1 << 1;

extern EntityPtr root;

struct MainCamera
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cCameraPtr camera = nullptr;

	void init(EntityPtr e);
};
extern MainCamera main_camera;

struct MainPlayer
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cNavAgentPtr nav_agent = nullptr;
	cCharacterPtr character = nullptr;

	void init(EntityPtr e);
};
extern MainPlayer main_player;

extern cCharacterPtr selecting_target;

/// Reflect ctor
struct cMain : Component
{
	/// Reflect requires
	cNodePtr node;

	/// Reflect
	float camera_length = 10.f;
	/// Reflect
	float camera_angle = 45.f;

	~cMain();
	void on_active() override;
	void on_inactive() override;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};

float random01();
