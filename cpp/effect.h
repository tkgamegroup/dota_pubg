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

extern std::vector<EffectPreset> effect_presets;

extern std::vector<cEffectPtr> effects;
extern std::vector<cEffectPtr> dead_effects;

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

	bool dead = false;

	~cEffect();
	void start() override;
	void update() override;
	void die();

	struct Create
	{
		virtual cEffectPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

void init_effects();
