#include "presets.h"

UnitInfosPreset unit_infos;
BuildingInfosPreset building_infos;

const BuildingInfo* BuildingInfosPreset::find(std::string_view name) const
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
	load_preset_file(L"assets\\unit_infos.preset", &unit_infos);
	load_preset_file(L"assets\\building_infos.preset", &building_infos);
}
