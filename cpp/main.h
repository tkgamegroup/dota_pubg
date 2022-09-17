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
	float camera_length = 15.f;
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

std::vector<cCharacterPtr> get_characters(const vec3& pos, float radius, uint faction);
void add_projectile(EntityPtr prefab, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, const std::function<void(cProjectilePtr)>& on_update = {});
void add_projectile(EntityPtr prefab, const vec3& pos, const vec3& location, float speed, float collide_radius, uint collide_faction, const std::function<void(cCharacterPtr)>& on_collide, const std::function<void(cProjectilePtr)>& on_update = {});
void add_chest(const vec3& pos, uint item_id, uint item_num = 1);
void teleport(cCharacterPtr character, const vec3& location);
