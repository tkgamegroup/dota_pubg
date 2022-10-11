#pragma once

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

enum Faction
{
	FactionCreep = 1 << 0,
	FactionParty1 = 1 << 1,
	FactionParty2 = 1 << 2,
	FactionParty3 = 1 << 3,
	FactionParty4 = 1 << 4
};

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

	std::vector<vec3> site_positions;
	std::vector<std::pair<float, int>> site_centrality;

	void init(EntityPtr e);
	vec3 get_coord(const vec2& uv);
	vec3 get_coord(const vec3& pos);
	vec3 get_coord_by_centrality(int i);
};
extern MainTerrain main_terrain;

struct MainPlayer
{
	uint faction;
	uint character_id;

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

extern std::map<uint, cCharacterPtr> characters_by_id;
extern std::map<uint, std::vector<cCharacterPtr>> characters_by_faction;
extern std::map<uint, cProjectilePtr> projectiles_by_id;
extern std::map<uint, cChestPtr> chests_by_id;

EntityPtr get_prefab(const std::filesystem::path& path);
void add_player(vec3& pos, uint& faction, uint& preset_id);
std::vector<cCharacterPtr> find_characters(const vec3& pos, float radius, uint faction);
cCharacterPtr add_character(uint preset_id, const vec3& pos, uint faction, uint id = 0);
void remove_character(uint id);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, const std::function<void(cProjectilePtr)>& on_update = {}, uint id = 0);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, const vec3& location, float speed, uint collide_faction, const std::function<void(cCharacterPtr)>& on_collide, uint id = 0);
void remove_projectile(uint id);
cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num = 1, uint id = 0);
void remove_chest(uint id);
void teleport(cCharacterPtr character, const vec3& location);
