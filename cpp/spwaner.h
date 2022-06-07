#pragma once

#include "main.h"

/// Reflect ctor
struct cSpwaner : Component
{
	/// Reflect requires
	cNodePtr node;
	/// Reflect requires
	cNavAgentPtr nav_agent;

	/// Reflect
	float spwan_interval = 10.f;
	/// Reflect
	std::filesystem::path prefab_path;
	/// Reflect
	void set_prefab_path(const std::filesystem::path& path);

	float spwan_timer = 0.f;

	EntityPtr prefab = nullptr;
	float prefab_radius = 0.f;

	void update() override;

	struct Create
	{
		virtual cSpwanerPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
