#include "effect.h"

#include <flame/universe/components/audio_source.h>

std::vector<EffectPreset> effect_presets;
static EffectPreset dummy_preset;

void init_effects()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\effects.ini")).sections)
	{
		auto& preset = effect_presets.emplace_back();
		preset.id = effect_presets.size() - 1;
		preset.name = section.name;
		for (auto& e : section.entries)
		{
			switch (e.key_hash)
			{
			case "path"_h:
				preset.path = e.values[0];
				break;
			case "sound_path"_h:
				preset.sound_path = e.values[0];
				break;
			}
		}
	}
}

int EffectPreset::find(const std::string& name)
{
	for (auto i = 0; i < effect_presets.size(); i++)
	{
		if (effect_presets[i].name == name)
			return i;
	}
	return -1;
}

const EffectPreset& EffectPreset::get(uint id)
{
	return effect_presets[id];
}

std::vector<cEffectPtr> effects;
std::vector<cEffectPtr> dead_effects;

cEffect::~cEffect()
{
	std::erase_if(effects, [this](const auto& i) {
		return i == this;
	});
}

void cEffect::start()
{
	timer = duration;

	if (!preset)
		preset = &dummy_preset;

	std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
	if (!preset->sound_path.empty())
		audio_buffer_names.emplace_back(preset->sound_path, "start");
	audio_source->set_buffer_names(audio_buffer_names);
	audio_source->play("start"_h);
}

void cEffect::update()
{
	if (dead)
		return;

	if (timer > 0.f)
	{
		timer -= delta_time;
		if (timer < 0.f)
			die();
	}
}

void cEffect::die()
{
	if (dead)
		return;

	dead_effects.push_back(this);
	dead = true;
}

struct cEffectCreate : cEffect::Create
{
	cEffectPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cEffect;
	}
}cEffect_create;
cEffect::Create& cEffect::create = cEffect_create;
