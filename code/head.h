#pragma once

#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>

using namespace flame;

FLAME_TYPE(cLauncher)
FLAME_TYPE(cGame)
FLAME_TYPE(cItem)
FLAME_TYPE(cAbility)
FLAME_TYPE(cTalent)
FLAME_TYPE(cBuff)
FLAME_TYPE(cObject)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cEffect)
FLAME_TYPE(cSectorCollider)
FLAME_TYPE(cChest)
FLAME_TYPE(cAI)
FLAME_TYPE(cNWDataHarvester)

const auto CharacterTag = TagUser;

// Reflect
enum FactionFlags
{
	FactionNone = 0,
	FactionCreep = 1 << 0,
	FactionParty1 = 1 << 1,
	FactionParty2 = 1 << 2,
	FactionParty3 = 1 << 3,
	FactionParty4 = 1 << 4
};

inline FactionFlags operator| (FactionFlags a, FactionFlags b) { return (FactionFlags)((int)a | (int)b); }
inline FactionFlags operator~ (FactionFlags v) { return (FactionFlags)(~(int)v); }

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

// Reflect
enum CharacterState
{
	CharacterStateNormal = 0,
	CharacterStateStun = 1 << 0,
	CharacterStateRoot = 1 << 1,
	CharacterStateSilence = 1 << 2
};

enum CharacterMessage
{
	CharacterLevelUp,
	CharacterGainItem,
	CharacterGainAbility,
	CharacterAbilityLevelUp,
};

enum MultiPlayerType
{
	SinglePlayer,
	MultiPlayerAsHost,
	MultiPlayerAsClient
};

// Reflect
enum UnitType
{
	UnitCampCreep,
	UnitLaneCreep,
	UnitDefenseTower
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

template<typename T>
struct Tracker
{
	uint hash;
	T obj = nullptr;

	Tracker()
	{
		hash = rand();
	}

	~Tracker()
	{
		if (obj)
			obj->entity->message_listeners.remove(hash);
	}

	void set(T oth)
	{
		if (obj)
			obj->entity->message_listeners.remove((uint)this);
		obj = oth;
		if (oth)
		{
			oth->entity->message_listeners.add([this](uint h, void*, void*) {
				if (h == "destroyed"_h)
					obj = nullptr;
				}, hash);
		}
	}
};

extern float gtime;
extern MultiPlayerType multi_player;

bool parse_literal(const std::string& str, int& id);
