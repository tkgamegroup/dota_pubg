#pragma once

#include "main.h"

struct ProjectilePreset
{
	uint					id;
	std::string				name;
	std::filesystem::path	path;

	float collide_radius = 0.f;

	void(*update)(cProjectilePtr) = nullptr;

	static int find(const std::string& name);
	static const ProjectilePreset& get(uint id);
};

/// Reflect ctor
struct cProjectile : Component
{
	/// Reflect requires
	cNodePtr node;

	uint id;
	uint preset_id;
	inline const ProjectilePreset& get_preset()
	{
		return ProjectilePreset::get(preset_id);
	}

	float speed = 0.1f;

	bool					use_target = true;
	vec3					location;
	Tracker<cCharacterPtr>	target;

	float collide_radius = 0.f;
	uint collide_faction = 0;

	std::function<void(cCharacterPtr c)>				on_collide;
	std::function<void(const vec3& l, cCharacterPtr t)> on_end;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cProjectilePtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
 