#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)
FLAME_TYPE(cPlayer)
FLAME_TYPE(cCharacter)

const auto CharacterTag = 1 << 1;

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
	cPlayerPtr player = nullptr;

	void init(EntityPtr e);
};
extern MainPlayer main_player;

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

float random01();
