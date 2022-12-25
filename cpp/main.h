#pragma once

#include <flame/universe/component.h>
#include <flame/universe/entity.h>

using namespace flame;

FLAME_TYPE(cLauncher)
FLAME_TYPE(cMain)
FLAME_TYPE(cObject)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cEffect)
FLAME_TYPE(cChest)
FLAME_TYPE(cCreepAI)
FLAME_TYPE(cNWDataHarvester)

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
	cTerrainPtr hf_terrain = nullptr; // height field terrain
	cVolumePtr mc_terrain = nullptr; // marching cubes terrain
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

struct ImDrawList;
struct Shortcut
{
	int id = -1;
	KeyboardKey key = KeyboardKey_Count;

	virtual void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) {}
	virtual void click() {}
};

struct ItemInstance;
struct ItemShortcut : Shortcut
{
	ItemInstance* ins;

	ItemShortcut(ItemInstance* ins);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

struct AbilityInstance;
struct AbilityShortcut : Shortcut
{
	AbilityInstance* ins;

	AbilityShortcut(AbilityInstance* ins);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

extern std::unique_ptr<Shortcut> shortcuts[10];

// Reflect ctor
struct cMain : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float camera_length = 15.f;
	// Reflect
	float camera_angle = 45.f;

	~cMain();

	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

void enable_game(bool v);
EntityPtr get_prefab(const std::filesystem::path& path);
void add_player(vec3& pos, uint& faction, uint& preset_id);
std::vector<cCharacterPtr> find_characters(const vec3& pos, float radius, uint faction);
cCharacterPtr add_character(uint preset_id, const vec3& pos, uint faction, uint id = 0);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, const vec3& location, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cEffectPtr add_effect(uint preset_id, const vec3& pos, const vec3& eul, float duration, uint id = 0);
cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num = 1, uint id = 0);
void teleport(cCharacterPtr character, const vec3& location);

void add_floating_tip(const vec3& pos, const std::string& text, const cvec4& color);
