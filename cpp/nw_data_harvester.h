#pragma once

#include "main.h"

/// Reflect ctor
struct cNWDataHarvester : Component
{
	std::unordered_map<uint, uint/*factions flags: is this var needs to sync to those factions*/> targets;

	void add_target(uint var);

	void on_init() override;

	struct Create
	{
		virtual cNWDataHarvesterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
