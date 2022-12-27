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
		if (std::isdigit(sp[i][0]))
		{
			auto sp2 = SUS::split(sp[i], ',');
			auto& vec = internal_parameters[i - 1];
			vec.resize(sp2.size());
			for (auto j = 0; j < vec.size(); j++)
			{
				switch (sp2[j].back())
				{
				case 'f':
					vec[j].f = s2t<float>(sp2[j]);
					break;
				case 'u':
					vec[j].f = s2t<uint>(sp2[j]);
					break;
				default:
					vec[j].i = s2t<int>(sp2[j]);
				}
			}
			parameter_names.push_back(0);
		}
		else
			parameter_names.push_back(sh(sp[i].c_str()));
	}
}

void Command::execute(cCharacterPtr character, const Parameters& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint idx, uint lv)->const Parameter& {
		auto name = parameter_names[idx];
		if (name == 0)
			return internal_parameters.at(idx)[lv];
		return external_parameters.at(name)[lv];
	};

	switch (type)
	{
	case tRestoreHP:
		character->restore_hp(read_parameter(0, lv).i);
		break;
	case tRestoreMP:
		character->restore_mp(read_parameter(0, lv).i);
		break;
	case tLevelUp:
		character->gain_exp(character->exp_max);
		break;
	}
}
