#pragma once

#include "main.h"

/// Reflect ctor
struct cNWDataHarvester : Component
{
	std::vector<std::unordered_map<uint/*var hash*/, std::pair<uint, uint>/*the current and reset faction flags (is this var needs to sync to those factions)*/>> targets;

	void add_target(uint comp, uint var, uint flags = 0xffffffff);

	void on_init() override;

	struct Create
	{
		virtual cNWDataHarvesterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
