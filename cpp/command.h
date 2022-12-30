#pragma once

#include "main.h"

union Variant
{
	float f;
	uint u;
	int i;
};

typedef std::unordered_map<uint, std::vector<Variant>>	ParameterPack;
typedef std::vector<std::pair<std::string, uint>>		ParameterNames;

Variant read_variant(std::string& str);

void read_parameter_values(std::vector<Variant>& vec, const std::string& text);

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::string& text);

struct Command
{
	// Reflect
	enum Type
	{
		tNull,
		tRollDice100,
		tRestoreHP,
		tRestoreMP,
		tTakeDamage,
		tTakeDamagePct,
		tLevelUp,
		tIncreaseHPMax,
		tIncreaseMPMax,
		tIncreaseATK,
		tIncreasePHYDEF,
		tIncreaseMAGDEF,
		tIncreaseHPREG,
		tIncreaseMPREG,
		tIncreaseMOVSP,
		tIncreaseATKSP,
		tIncreaseHPMaxPct,
		tIncreaseMPMaxPct,
		tIncreaseATKPct,
		tAddBuff,
		tAddBuffToTarget,
		tAddAttackEffect,
	};

	Type type = tNull;
	std::vector<std::pair<char, Variant>> parameters; // first: 0 use internal (the second), 1 use external

	Command() {}
	Command(std::vector<std::string>& tokens);
	void execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const;
};

typedef std::vector<Command> CommandList;

void parse_command_list(CommandList& list, const std::string& text);
