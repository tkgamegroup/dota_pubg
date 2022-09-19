#pragma once

#include "main.h"

/// Reflect ctor
struct cCreepAI : Component
{
	/// Reflect requires
	cCharacterPtr character;

	void update() override;
};
