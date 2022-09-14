#pragma once

#include "main.h"
#include "item.h"

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

struct ItemInstance
{
	uint id;
	uint num = 1;
};

struct AbilityInstance
{
	uint id;
	uint lv = 0;
	float cd_max = 0.f;
	float cd_timer = 0.f;
};

struct BuffInstance
{
	uint id;
	float timer;
};

struct Command
{
	cCharacterPtr character;

	Command(cCharacterPtr character);
	virtual ~Command() {}
	virtual void update() = 0;
};

struct CommandIdle : Command
{
	CommandIdle(cCharacterPtr character);

	void update() override;
};

struct CommandMoveTo : Command
{
	vec3 location;

	CommandMoveTo(cCharacterPtr character, const vec3& _location);

	void update() override;
};

struct CommandAttackTarget : Command
{
	Tracker<cCharacterPtr> target;

	CommandAttackTarget(cCharacterPtr character, cCharacterPtr _target);

	void update() override;
};

struct CommandAttackLocation : Command
{
	Tracker<cCharacterPtr> target;
	vec3 location;

	CommandAttackLocation(cCharacterPtr character, const vec3& _location);

	void update() override;
};

struct CommandPickUp : Command
{
	Tracker<cChestPtr> target;

	CommandPickUp(cCharacterPtr character, cChestPtr _target);

	void update() override;
};

struct CommandCastAbility : Command
{
	AbilityInstance* ins;

	CommandCastAbility(cCharacterPtr character, AbilityInstance* ins);

	void update() override;
};

struct CommandCastAbilityToLocation : Command
{
	vec3 location;
	AbilityInstance* ins;

	CommandCastAbilityToLocation(cCharacterPtr character, AbilityInstance* ins, const vec3& _location);

	void update() override;
};

struct CommandCastAbilityToTarget : Command
{
	Tracker<cCharacterPtr> target;
	AbilityInstance* ins;

	CommandCastAbilityToTarget(cCharacterPtr character, AbilityInstance* ins, cCharacterPtr _target);

	void update() override;
};

enum Action
{
	ActionNone,
	ActionMove,
	ActionAttack,
	ActionCast
};

struct CharacterPreset
{
	uint					id;
	std::string				name;

	uint exp_base = 0;
	uint hp = 1000;
	uint mp = 1000;
	uint VIG = 0; // vigor
	uint MND = 0; // mind
	uint STR = 0; // strength
	uint DEX = 0; // dexterity
	uint INT = 0; // intelligence
	uint LUK = 0; // luck
	uint atk = 0;
	float atk_distance = 1.5f;
	float atk_time = 2.f; // attack interval
	float atk_point = 1.f; // hit point
	std::filesystem::path atk_projectile_name;
	EntityPtr atk_projectile = nullptr;
	float cast_time = 1.f; // animation time
	float cast_point = 0.5f; // hit point
	uint phy_def = 0; // physical defense
	uint mag_def = 0; // magic defense
	uint hp_reg = 0; // hp regeneration
	uint mp_reg = 0; // mp regeneration
	uint mov_sp = 100; // move speed
	uint atk_sp = 100; // attack speed

	static int find(const std::string& name);
	static const CharacterPreset& get(uint id);
};

enum State
{
	StateNormal = 0,
	StateStun = 1 << 0,
	StateRoot = 1 << 1,
	StateSilence = 1 << 2
};

/// Reflect ctor
struct cCharacter : Component
{
	/// Reflect requires
	cNodePtr node;
	/// Reflect requires
	cNavAgentPtr nav_agent;

	cArmaturePtr armature = nullptr;

	/// Reflect
	uint faction = 0;
	/// Reflect
	void set_faction(uint _faction);
	/// Reflect
	uint ai_id = 0;
	/// Reflect
	uint preset_id = 0;
	/// Reflect
	std::string preset_name;
	/// Reflect
	void set_preset_name(const std::string& name);
	inline const CharacterPreset& get_preset()
	{
		return CharacterPreset::get(preset_id);
	}

	/// Reflect
	uint lv = 1;
	/// Reflect
	uint exp = 0;
	uint exp_max = 0;

	State state = StateNormal;

	/// Reflect
	uint hp = 1000;
	uint hp_max = 1000;

	/// Reflect
	uint mp = 10;
	uint mp_max = 10;

	uint VIG = 0;
	uint MND = 0;
	uint STR = 0;
	uint DEX = 0;
	uint INT = 0;
	uint LUK = 0;

	uint atk = 10;
	uint phy_def = 0;
	uint mag_def = 0;
	uint hp_reg = 0;
	uint mp_reg = 0;
	uint mov_sp = 0;
	uint atk_sp = 0;

	uint abilities_points = 0;
	std::vector<std::unique_ptr<AbilityInstance>>	abilities;
	std::vector<std::unique_ptr<ItemInstance>>		inventory;
	int												equipments[EquipPart_Count];
	std::vector<std::unique_ptr<BuffInstance>>		buffs;

	bool dead = false;
	bool stats_dirty = true;
	std::unique_ptr<Command> command;
	Action action = ActionNone;
	float move_speed = 1.f;
	float attack_speed = 1.f;
	float cast_speed = 1.f;
	float regeneration_timer = 1.f;
	float search_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_timer = 0.f;
	float cast_timer = 0.f;

	~cCharacter();
	void on_init() override;

	void inflict_damage(cCharacterPtr target, uint value);
	bool take_damage(uint value); // return true if the damage causes the character die
	void gain_exp(uint v);
	bool gain_item(uint id, uint num);
	bool gain_ability(uint id);
	void use_item(ItemInstance* ins);
	void cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target);
	void add_buff(uint id, float time);
	void die();

	bool process_approach(const vec3& target, float dist = 0.1f, float ang = 0.f);
	void process_attack_target(cCharacterPtr target);
	void process_cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target);

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
