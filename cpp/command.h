#pragma once

#include "main.h"

#include <flame/foundation/typeinfo.h>

struct Parameter
{
	enum Type
	{
		tEmpty,
		tImmediate,
		tExternal,
		tVariable,
		tExpression
	};

	enum vType
	{
		vInt,
		vUint,
		vFloat,
		vPercentage
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

	inline int to_i() const
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

	inline float to_f() const
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

	inline std::string to_str() const
	{
		switch (vt)
		{
		case vInt: return std::format("{}", u.v.i);
		case vUint: return std::format("{}", u.v.u);
		case vFloat: return std::format("{.1f}", u.v.f);
		case vPercentage: return std::format("{}%", u.v.i);
		}
		return "";
	}
};

typedef std::unordered_map<uint, std::vector<Parameter>>	ParameterPack;
typedef std::vector<std::pair<std::string, uint>>			ParameterNames;

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
		cBitInverse,
		cIfEqual,
		cIfNotEqual,
		cLoop,
		cBreak,
		cGenerateRnd,
		cRollDice100,
		cWait,
		cGetFaction,
		cForNearbyEnemies, //
		cNearestCharacter,
		cRestoreHP,
		cRestoreMP,
		cTakeDamage,
		cInflictDamage,
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
		cAddState,
		cAddBuff,
		cAddAttackEffect,
		cSetSectorCollideCallback,
		cTeleportToTarget, //
		cAddEffect,
		cAddEffectToCharacter, //
		cAddEffectFaceTarget, //
	};

	// Reflect
	enum Variable
	{
		vCharacter,
		vTargetCharacter,
		vTargetPos,
		vREG0,
		vREG1,
		vREG2,
		vREG3,
		vREG4,
		vREG5,
		vREG6,
		vREG7,
		vZeroREG
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
