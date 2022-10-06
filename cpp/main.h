#pragma once

#include <flame/foundation/network.h>
#include <flame/universe/component.h>
#include <flame/universe/entity.h>

using namespace flame;

FLAME_TYPE(cLauncher)
FLAME_TYPE(cMain)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cSpwaner)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cChest)
FLAME_TYPE(cCreepAI)

const auto CharacterTag = 1 << 1;

enum TargetType
{
	TargetNull = 0,
	TargetEnemy = 1 << 0,
	TargetFriendly = 1 << 1,
	TargetLocation = 1 << 2
};

enum DamageType
{
	PhysicalDamage,
	MagicDamage
};

enum MultiPlayerType
{
	SinglePlayer,
	MultiPlayerAsHost,
	MultiPlayerAsClient
};

extern MultiPlayerType multi_player;
extern network::ClientPtr nw_client;
extern network::ServerPtr nw_server;

template<typename T>
struct Tracker
{
	uint hash;
	T obj = nullptr;

	Tracker()
	{
		hash = rand();
	}

	~Tracker()
	{
		if (obj)
			obj->entity->message_listeners.remove(hash);
	}

	void set(T oth)
	{
		if (obj)
			obj->entity->message_listeners.remove((uint)this);
		obj = oth;
		if (oth)
		{
			oth->entity->message_listeners.add([this](uint h, void*, void*) {
				if (h == "destroyed"_h)
					obj = nullptr;
			}, hash);
		}
	}
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

bool get_vision(const vec3& coord);
std::vector<cCharacterPtr> get_characters(const vec3& pos, float radius, uint faction);
cCharacterPtr add_character(EntityPtr prefab, const vec3& pos, uint faction, const std::string& guid = "");
cProjectilePtr add_projectile(EntityPtr prefab, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, const std::function<void(cProjectilePtr)>& on_update = {});
cProjectilePtr add_projectile(EntityPtr prefab, const vec3& pos, const vec3& location, float speed, float collide_radius, uint collide_faction, const std::function<void(cCharacterPtr)>& on_collide, const std::function<void(cProjectilePtr)>& on_update = {});
cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num = 1);
void teleport(cCharacterPtr character, const vec3& location);
