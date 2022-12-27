#include "command.h"
#include "character.h"

Command::Command(const std::string& line)
{

}

void Command::execute(cCharacterPtr character, const Parameters& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint idx, uint lv)->const Parameter& {
		auto name = parameter_names[idx];
		if (name == 0)
			return internal_parameters[idx][lv];
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
