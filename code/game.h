#pragma once

#include "head.h"

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
	uint faction;
	uint character_id;

	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cNavAgentPtr nav_agent = nullptr;
	cCharacterPtr character = nullptr;

	void init(EntityPtr e);
};
extern MainPlayer main_player;

void enable_game(bool v);
EntityPtr get_prefab(const std::filesystem::path& path);
std::filesystem::path get_prefab_path(uint prefab_id /* path hash */ );
void add_player(vec3& pos, uint& faction, uint& prefab_id);
std::vector<cCharacterPtr> find_characters(FactionFlags faction, const vec3& pos, float r1, float r0 = 0.f, float central_angle = 360.f, float direction_angle = 0.f);
cCharacterPtr add_character(const std::filesystem::path& prefab_path, const vec3& pos, FactionFlags faction, uint id = 0);
cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, const vec3& location, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cEffectPtr add_effect(const std::filesystem::path& prefab_path, const vec3& pos, const vec3& eul, float duration, uint id = 0);
cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num = 1, uint id = 0);
void teleport(cCharacterPtr character, const vec3& location);

// Reflect ctor
struct cGame : Component
{
	// Reflect requires
	cNodePtr node;

	// Reflect
	float camera_length = 15.f;
	// Reflect
	float camera_angle = 45.f;
	// Reflect
	bool enable_control = true;

	~cGame();

	void start() override;
	void update() override;

	struct Create
	{
		virtual cGamePtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

// Reflect ctor
struct cLauncher : Component
{
	~cLauncher();

	void start() override;

	struct Create
	{
		virtual cLauncherPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

extern "C" EXPORT void* cpp_info();
