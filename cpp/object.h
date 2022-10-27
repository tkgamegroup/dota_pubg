#pragma once

#include "main.h"

extern std::map<uint, cObjectPtr> objects;

/// Reflect ctor
struct cObject : Component
{
	/// Reflect
	uint visible_flags = 0;
	/// Reflect
	void set_visible_flags(uint v);

	uint preset_id = 0;

	uint uid = 0;
	void set_uid(uint id = 0);

	~cObject();

	struct Create
	{
		virtual cObjectPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
