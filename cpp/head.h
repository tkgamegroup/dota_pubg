#pragma once

#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>

using namespace flame;

FLAME_TYPE(cLauncher)
FLAME_TYPE(cGame)
FLAME_TYPE(cObject)
FLAME_TYPE(cCharacter)
FLAME_TYPE(cProjectile)
FLAME_TYPE(cEffect)
FLAME_TYPE(cSectorCollider)
FLAME_TYPE(cChest)
FLAME_TYPE(cCreepAI)
FLAME_TYPE(cNWDataHarvester)

struct AbilityInstance;
struct ItemInstance;
struct BuffInstance;

const auto CharacterTag = TagUser;

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
extern bool in_editor;
extern MultiPlayerType multi_player;

bool parse_literal(const std::string& str, int& id);
