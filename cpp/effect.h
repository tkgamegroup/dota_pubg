#pragma once

#include "main.h"

struct EffectPreset
{
	uint					id;
	std::string				name;
	std::filesystem::path	path;

	std::filesystem::path	sound_path;

	static int find(const std::string& name);
	static const EffectPreset& get(uint id);
};

// Reflect ctor
struct cEffect : Component
{
	// Reflect requires
	cNodePtr node;
	// Reflect requires
	cAudioSourcePtr audio_source;
	// Reflect auto_requires
	cObjectPtr object;

	const EffectPreset* preset = nullptr;

	float duration = 0.f;
	float timer = 0.f;

	void start() override;
	void update() override;

	struct Create
	{
		virtual cEffectPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
