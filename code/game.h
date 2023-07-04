#pragma once

#include "head.h"

extern EntityPtr root;

struct MainCamera
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cCameraPtr camera = nullptr;

	vec3 follow_target = vec3(0.f, -2000.f, 0.f);

	void init(EntityPtr e);
	void set_follow_target(const AABB& bounds);
	void update();
};
extern MainCamera main_camera;

void enable_game(bool v);
EntityPtr get_prefab(const std::filesystem::path& path);
std::filesystem::path get_prefab_path(uint prefab_id /* path hash */ );
void add_player(vec3& pos, uint& faction, uint& prefab_id);
std::vector<cCharacterPtr> find_characters_within_circle(FactionFlags faction, const vec3& pos, float radius);
std::vector<cCharacterPtr> find_characters_within_sector(FactionFlags faction, const vec3& pos, float inner_radius, float outer_radius, float angle, float rotation);
std::vector<cCharacterPtr> find_characters_within_camera(FactionFlags faction);
cCharacterPtr add_character(const std::filesystem::path& prefab_path, const vec3& pos, FactionFlags faction, uint id = 0);
cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, cCharacterPtr target, float speed, uint id = 0);
cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, const vec3& location, float speed, uint id = 0);
cEffectPtr add_effect(const std::filesystem::path& prefab_path, const vec3& pos, const quat& qut, float duration, uint id = 0);
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
	// Reflect
	bool enable_ui = true;
	// Reflect
	bool get_enable_collider_debugging();
	// Reflect
	void set_enable_collider_debugging(bool v);
	// Reflect
	bool wtf = false;

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
