#include "command.h"
#include "character.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>

Command::Command(const std::string& line)
{
	auto sp = SUS::split(line);
	TypeInfo::unserialize_t(sp[0], type);
	for (auto i = 1; i < sp.size(); i++)
	{
		auto name = 0;
		auto& p = sp[i];
		if (std::isdigit(p[0]))
		{
			auto& vec = internal_parameters[i - 1];
			read_parameter_values(vec, p);
			parameter_names.push_back(0);
		}
		else
			parameter_names.push_back(sh(p.c_str()));
	}
}

void Command::execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const Parameters& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint idx, uint lv)->const Parameter& {
		auto name = parameter_names[idx];
		if (name == 0)
		{
			auto& vec = internal_parameters.at(idx);
			return lv <= vec.size() ? vec[lv] : vec[0];
		}
		auto& vec = external_parameters.at(name);
		return lv <= vec.size() ? vec[lv] : vec[0];
	};

	switch (type)
	{
	case tRestoreHP:
		character->restore_hp(read_parameter(0, lv).i);
		break;
	case tRestoreMP:
		character->restore_mp(read_parameter(0, lv).i);
		break;
	case tTakeDamage:
		break;
	case tTakeDamagePct:
		character->take_damage(MagicDamage, character->hp_max * (read_parameter(0, lv).i / 100.f));
		break;
	case tLevelUp:
		character->gain_exp(character->exp_max);
		break;
	case tIncreaseHPMax:
		break;
	case tIncreaseMPMax:
		break;
	case tIncreaseATK:
		character->atk += read_parameter(0, lv).i;
		break;
	case tIncreaseMOVSP:
		character->mov_sp += read_parameter(0, lv).i;
		break;
	case tIncreaseATKSP:
		character->atk_sp += read_parameter(0, lv).i;
		break;
	case tIncreaseHPMaxPct:
		character->hp_max *= (100 + read_parameter(0, lv).i) / 100.f;
		break;
	case tIncreaseMPMaxPct:
		break;
	case tIncreaseATKPct:
		character->atk *= (100 + read_parameter(0, lv).i) / 100.f;
		break;
	}
}
