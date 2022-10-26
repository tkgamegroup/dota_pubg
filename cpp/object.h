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

	uint uid = 0;

	~cObject();

	struct Create
	{
		virtual cObjectPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};

void add_object(uint id = 0);
