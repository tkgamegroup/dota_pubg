#pragma once

#include <flame/universe/component.h>

bool parse_literal(const std::string& str, int& id);

extern bool in_editor;
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

	void on_active() override;
	void on_inactive() override;
	void start() override;
	void update() override;

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

extern float gtime;

void enable_game(bool v);
EntityPtr get_prefab(const std::filesystem::path& path);
void add_player(vec3& pos, uint& faction, uint& preset_id);
std::vector<cCharacterPtr> find_characters(uint faction, const vec3& pos, float r1, float r0 = 0.f, float central_angle = 360.f, float direction_angle = 0.f);
cCharacterPtr add_character(uint preset_id, const vec3& pos, uint faction, uint id = 0);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cProjectilePtr add_projectile(uint preset_id, const vec3& pos, const vec3& location, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id = 0);
cEffectPtr add_effect(uint preset_id, const vec3& pos, const vec3& eul, float duration, uint id = 0);
cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num = 1, uint id = 0);
void teleport(cCharacterPtr character, const vec3& location);

