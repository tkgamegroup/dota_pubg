#pragma once

#include "main.h"

/// Reflect ctor
struct cLauncher : Component
{

	~cLauncher();
	void start() override;

	struct Create
	{
		virtual cLauncherPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
