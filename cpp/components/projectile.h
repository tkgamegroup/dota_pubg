#pragma once

#include "main.h"

struct ProjectilePreset
{
	uint					id;
	std::string				name;
	std::filesystem::path	path;

	static int find(const std::string& name);
	static const ProjectilePreset& get(uint id);
};

extern std::vector<ProjectilePreset> projectile_presets;

extern std::vector<cProjectilePtr> projectiles;
extern std::vector<cProjectilePtr> dead_projectiles;
extern bool removing_dead_projectiles;

// Reflect ctor
struct cProjectile : Component
{
	// Reflect requires
	cNodePtr node;
	// Reflect auto_requires
	cObjectPtr object;

	const ProjectilePreset* preset = nullptr;

	float speed = 0.1f;

	bool					use_target = true;
	vec3					location;
	Tracker<cCharacterPtr>	target;

	std::function<void(const vec3& l, cCharacterPtr t)> on_end;

	bool dead = false;

	~cProjectile();
	void update() override;
	void die();

	struct Create
	{
		virtual cProjectilePtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

void init_projectiles();
 