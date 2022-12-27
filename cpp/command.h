#pragma once

#include "main.h"

union Parameter
{
	float f;
	uint u;
	int i;
};

typedef std::unordered_map<uint, std::vector<Parameter>> Parameters;

struct Command
{
	enum Type
	{
		tRestoreHP,
		tRestoreMP,
		tLevelUp,
		tIncreaseHPMax,
		tIncreaseMPMax
	};

	Type type;
	std::vector<uint> parameter_names; // 0 to internal, or to external
	Parameters internal_parameters;

	Command(const std::string& line);
	void execute(cCharacterPtr character, const Parameters& external_parameters, uint lv) const;
};
