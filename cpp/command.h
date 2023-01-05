#pragma once

#include "main.h"

#include <flame/foundation/typeinfo.h>

struct Parameter
{
	enum Type
	{
		tImmediate,
		tExternal,
		tSpecialVariable,
		tExpression
	};

	enum vType
	{
		vInt,
		vUint,
		vFloat,
		vPercentage
	};

	// Reflect
	enum SpecialVariable
	{
		sTargetCharacter,
		sREG0,
		sREG1,
		sREG2,
		sREG3,
	};

	enum Operator
	{
		OpNull,
		OpAdd,
		OpMinus,
		OpMultiply,
		OpDivide
	};

	struct Expression
	{
		Operator op : 8 = OpNull;
		uint num_operand : 8 = 2;
	};

	Type type : 8 = tImmediate;
	vType vt : 8 = vInt;
	union
	{
		sVariant v;
		Expression e;
	}u = { .v={.i=0} };

	Parameter() {}
	Parameter(int v) { vt = vInt; u.v.i = v; }
	Parameter(uint v) { vt = vUint; u.v.u = v; }
	Parameter(float v) { vt = vFloat; u.v.f = v; }
	Parameter(const std::string& str);

	inline int to_i()
	{
		switch (vt)
		{
		case vInt: return u.v.i;
		case vUint: return u.v.u;
		case vFloat: return u.v.f;
		case vPercentage: return u.v.i;
		}
		return 0;
	}

	inline float to_f()
	{
		switch (vt)
		{
		case vInt: return u.v.i;
		case vUint: return u.v.u;
		case vFloat: return u.v.f;
		case vPercentage: return u.v.i / 100.f;
		}
		return 0.f;
	}
};

typedef std::unordered_map<uint, std::vector<Parameter>>	ParameterPack;
typedef std::vector<std::pair<std::string, uint>>			ParameterNames;

void read_parameter_values(std::vector<Parameter>& vec, const std::string& text);

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::vector<std::string>& tokens);

struct CommandList
{
	// Reflect
	enum Command
	{
		cNull,
		cBeginSub,
		cEndSub,
		cStore,
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
		cAddEffect,
		cAddEffectFaceTarget,
	};

	std::vector<std::pair<Command, std::vector<Parameter>>> cmds;
	std::unordered_map<uint, uint> sub_groups;

	void init_sub_groups();
	void build(const std::vector<std::string>& tokens);
	void execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const;

	inline operator bool() const
	{
		return !cmds.empty();
	}
};
