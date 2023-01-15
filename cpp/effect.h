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

struct SpecialEffect
{
	virtual ~SpecialEffect() {}
	virtual void update() = 0;
};

struct LinkEffect : SpecialEffect
{
	cEffectPtr effect;
	vec3 target_pos = vec3(0.f);

	LinkEffect(cEffectPtr effect);
	void update() override;
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
	// cEffect may control the particle system
	cParticleSystemPtr particle_system = nullptr;

	// Reflect hash=Normal|Link
	uint type = "Normal"_h;

	const EffectPreset* preset = nullptr;

	float duration = 0.f;
	float timer = 0.f;
	bool dead = false;

	std::unique_ptr<SpecialEffect> special_effect;

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
