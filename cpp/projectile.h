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

	void update() override;

	struct Create
	{
		virtual cProjectilePtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

void init_projectiles();
 