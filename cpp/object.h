#pragma once

#include "main.h"

/// Reflect ctor
struct cObject : Component
{
	/// Reflect
	uint visible_flags = 0;
	/// Reflect
	void set_visible_flags(uint v);

	struct Create
	{
		virtual cObjectPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
