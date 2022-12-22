#include "audio.h"

std::vector<AudioPreset> audio_presets;

void load_audio_presets()
{
	{
		auto& preset = audio_presets.emplace_back();
		preset.id = audio_presets.size() - 1;
		preset.path = L"assets\\level_up.wav";
		preset.name = "Level Up";
	}
	{
		auto& preset = audio_presets.emplace_back();
		preset.id = audio_presets.size() - 1;
		preset.path = L"assets\\characters\\dragon_knight\\attack_precast.wav";
		preset.name = "Dragon Knight Attack Precast";
	}
	{
		auto& preset = audio_presets.emplace_back();
		preset.id = audio_presets.size() - 1;
		preset.path = L"assets\\characters\\dragon_knight\\attack_hit.wav";
		preset.name = "Dragon Knight Attack Hit";
	}
}

int AudioPreset::find(const std::string& name)
{
	if (audio_presets.empty())
		load_audio_presets();
	for (auto i = 0; i < audio_presets.size(); i++)
	{
		if (audio_presets[i].name == name)
			return i;
	}
	return -1;
}

const AudioPreset& AudioPreset::get(uint id)
{
	if (audio_presets.empty())
		load_audio_presets();
	return audio_presets[id];
}
