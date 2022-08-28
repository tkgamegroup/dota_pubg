#pragma once

#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cSpwaner)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cChest)

const auto CharacterTag = 1 << 1;

enum TargetType
{
	TargetNull = 0,
	TargetEnemy = 1 << 0,
	TargetFriendly = 1 << 1,
	TargetLocation = 1 << 2
};

extern EntityPtr root;

struct MainCamera
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cCameraPtr camera = nullptr;

	void init(EntityPtr e);
};
extern MainCamera main_camera;

struct MainTerrain
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cTerrainPtr terrain = nullptr;
	vec3 extent;

	void init(EntityPtr e);
	vec3 get_coord(const vec2& uv);
	vec3 get_coord(const vec3& pos);
};
extern MainTerrain main_terrain;

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

	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};

void add_projectile(EntityPtr prefab, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(cCharacterPtr t)>& cb);
void add_chest(const vec3& pos, uint item_id, uint item_num = 1);
void teleport(cCharacterPtr character, const vec3& location);
