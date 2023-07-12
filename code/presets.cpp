#include "presets.h"

CharacterInfosPreset character_infos;
ProjectileInfosPreset projectile_infos;
EffectInfosPreset effect_infos;
BuildingInfosPreset building_infos;
TownInfosPreset town_infos;

const CharacterInfo* CharacterInfosPreset::find(std::string_view name) const
{
	for (auto& i : infos)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}
static CharacterInfo default_character_info;

const ProjectileInfo* ProjectileInfosPreset::find(std::string_view name) const
{
	for (auto& i : infos)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

const EffectInfo* EffectInfosPreset::find(std::string_view name) const
{
	for (auto& i : infos)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

const BuildingInfo* BuildingInfosPreset::find(std::string_view name) const
{
	for (auto& i : infos)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

void TownInfo::init()
{
	for (auto& b : construction_actions)
	{
		if (auto building_info = building_infos.find(b.name); building_info)
		{
			for (auto& t : building_info->training_actions)
			{
				if (auto character_info = character_infos.find(t.name); character_info)
					all_trainings.push_back(character_info);
			}
		}
	}
}

const TownInfo* TownInfosPreset::find(std::string_view name) const
{
	for (auto& i : infos)
	{
		if (i.name == name)
			return &i;
	}
	return nullptr;
}

void init_presets()
{
	load_preset_file(L"assets\\character_infos.preset", &character_infos);
	load_preset_file(L"assets\\projectile_infos.preset", &projectile_infos);
	load_preset_file(L"assets\\effect_infos.preset", &effect_infos);
	load_preset_file(L"assets\\building_infos.preset", &building_infos);
	load_preset_file(L"assets\\town_infos.preset", &town_infos);
	for (auto& t : town_infos.infos)
		t.init();
}
