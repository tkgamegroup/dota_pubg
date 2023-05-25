#include "presets.h"

UnitInfosPreset unit_infos;
BuildingInfosPreset building_infos;

void init_presets()
{
	load_preset_file(L"assets\\unit_infos.preset", &unit_infos);
	for (auto& i : unit_infos.infos)
	{

	}
	load_preset_file(L"assets\\building_infos.preset", &building_infos);
}
