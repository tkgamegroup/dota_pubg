#pragma once

#include <flame/math.h>

using namespace flame;

FLAME_TYPE(cLauncher)
FLAME_TYPE(cMain)
FLAME_TYPE(cObject)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cEffect)
FLAME_TYPE(cSectorCollider)
FLAME_TYPE(cChest)
FLAME_TYPE(cCreepAI)
FLAME_TYPE(cNWDataHarvester)

const auto CharacterTag = 1 << 1;

// Reflect
enum Faction
{
	FactionCreep = 1 << 0,
	FactionParty1 = 1 << 1,
	FactionParty2 = 1 << 2,
	FactionParty3 = 1 << 3,
	FactionParty4 = 1 << 4
};

// Reflect
enum TargetType
{
	TargetNull = 0,
	TargetEnemy = 1 << 0,
	TargetFriendly = 1 << 1,
	TargetLocation = 1 << 2
};

// Reflect
enum DamageType
{
	PhysicalDamage,
	MagicDamage
};

struct IDAndPos
{
	uint id;
	vec3 pos;
};

struct ObjAndPosXZ
{
	void* obj;
	vec2 pos_xz;
};
