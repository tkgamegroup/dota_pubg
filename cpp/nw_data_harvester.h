#pragma once

#include "main.h"

/// Reflect ctor
struct cNWDataHarvester : Component
{
	/*first: var hash, second: the current and reset faction flags (is this var needs to sync to those factions)*/
	std::unordered_map<uint, std::pair<uint, uint>> targets;

	void add_target(uint var, uint flags = 0xffffffff);

	void on_init() override;

	struct Create
	{
		virtual cNWDataHarvesterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
