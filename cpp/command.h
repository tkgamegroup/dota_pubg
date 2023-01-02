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

Variant read_variant(const std::string& str);

void read_parameter_values(std::vector<Variant>& vec, const std::string& text);

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::vector<std::string>& tokens);

struct CommandList
{
	// Reflect
	enum Command
	{
		cNull,
		cBeginSub,
		cEndSub,
		cIfEqual,
		cRollDice100,
		cForNearbyEnemies,
		cRestoreHP,
		cRestoreMP,
		cTakeDamage,
		cTakeDamagePct,
		cInflictDamge,
		cLevelUp,
		cIncreaseHPMax,
		cIncreaseMPMax,
		cIncreaseATK,
		cIncreasePHYDEF,
		cIncreaseMAGDEF,
		cIncreaseHPREG,
		cIncreaseMPREG,
		cIncreaseMOVSP,
		cIncreaseATKSP,
		cIncreaseHPMaxPct,
		cIncreaseMPMaxPct,
		cIncreaseATKPct,
		cAddState,
		cAddBuff,
		cAddBuffToTarget,
		cAddAttackEffect,
		cTeleportToTarget,
	};

	std::vector<Variant> data;
	std::unordered_map<uint, uint> non_immediates;
	std::vector<std::pair<uint, uint>> cmds;
	std::unordered_map<uint, uint> sub_groups;

	void init_sub_groups();
	void build(const std::vector<std::string>& tokens);
	void execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const;

	inline operator bool() const
	{
		return !cmds.empty();
	}
};
