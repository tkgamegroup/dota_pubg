#pragma once

#include "../head.h"

extern std::map<uint, cObjectPtr>			objects;
extern std::vector<std::pair<uint, uint>>	new_objects; // only in host
extern std::vector<uint>					removed_objects; // only in host

// Reflect ctor
struct cObject : Component
{
	// Reflect
	uint visible_flags = 0;
	// Reflect
	void set_visible_flags(uint v);

	uint prefab_id = 0;
	uint uid = 0;
	void init(uint prefab_id, uint uid);

	~cObject();

	struct Create
	{
		virtual cObjectPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
