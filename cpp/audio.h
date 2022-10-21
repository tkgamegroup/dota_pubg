#pragma once

#include "main.h"

struct AudioPreset
{
	uint					id;
	std::string				name;
	std::filesystem::path	path;

	static int find(const std::string& name);
	static const AudioPreset& get(uint id);
};
