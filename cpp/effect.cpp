#include "effect.h"

#include <flame/universe/components/audio_source.h>

std::vector<EffectPreset> effect_presets;

void init_effects()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\effects.ini")).sections)
	{
		auto& preset = effect_presets.emplace_back();
		preset.id = effect_presets.size() - 1;
		preset.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "path")
				preset.path = e.values[0];
			else if (e.key == "sound_path")
				preset.sound_path = e.values[0];
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

void cEffect::start()
{
	timer = duration;

	std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
	if (!preset->sound_path.empty())
		audio_buffer_names.emplace_back(preset->sound_path, "start");
	audio_source->set_buffer_names(audio_buffer_names);
	audio_source->play("start"_h);
}

void cEffect::update()
{
	if (timer > 0.f)
	{
		timer -= delta_time;
		if (timer < 0.f)
		{
			add_event([this]() {
				entity->remove_from_parent();
				return false;
			});
		}
	}
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
