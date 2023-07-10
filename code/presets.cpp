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
}
