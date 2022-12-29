#pragma once

#include "main.h"

union Parameter
{
	float f;
	uint u;
	int i;
};

typedef std::unordered_map<uint, std::vector<Parameter>>	Parameters;
typedef std::vector<std::pair<std::string, uint>>			ParameterNames;

inline void read_parameter_values(std::vector<Parameter>& vec, const std::string& text)
{
	auto sp2 = SUS::split(text, '/');
	vec.resize(sp2.size());
	for (auto j = 0; j < vec.size(); j++)
	{
		auto& str = sp2[j];
		switch (str.back())
		{
		case 'f':
			str.pop_back();
			vec[j].f = s2t<float>(str);
			break;
		case 'u':
			str.pop_back();
			vec[j].f = s2t<uint>(str);
			break;
		case 'i':
			str.pop_back();
		default:
			vec[j].i = s2t<int>(str);
		}
	}
}

inline void read_parameters(ParameterNames& parameter_names, Parameters& parameters, const std::string& text)
{
	for (auto& p : SUS::split(text))
	{
		auto sp = SUS::split(p, ':');
		auto hash = sh(sp[0].c_str());
		parameter_names.emplace_back(sp[0], hash);
		auto& vec = parameters.emplace(hash, std::vector<Parameter>()).first->second;
		read_parameter_values(vec, sp[1]);
	}
}

struct Command
{
	// Reflect
	enum Type
	{
		tNull,
		tRestoreHP,
		tRestoreMP,
		tTakeDamage,
		tTakeDamagePct,
		tLevelUp,
		tIncreaseHPMax,
		tIncreaseMPMax,
		tIncreaseATK,
		tIncreaseMOVSP,
		tIncreaseATKSP,
		tIncreaseHPMaxPct,
		tIncreaseMPMaxPct,
		tIncreaseATKPct,
	};

	Type type = tNull;
	std::vector<uint> parameter_names; // 0 to internal, or to external
	Parameters internal_parameters;

	Command(const std::string& line);
	void execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const Parameters& external_parameters, uint lv) const;
};

typedef std::vector<Command> CommandList;

inline void parse_command_list(CommandList& list, const std::string& text)
{
	for (auto& c : SUS::split(text, '&'))
		list.push_back(Command(SUS::get_trimed(c)));
}
