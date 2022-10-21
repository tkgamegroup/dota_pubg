#pragma once

#include "main.h"
#include "ability.h"
#include "item.h"
#include "buff.h"

struct Command
{
	uint type;
	cCharacterPtr character;

	Command(uint type, cCharacterPtr character);
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
	std::filesystem::path	path;

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
	int atk_projectile_preset = -1;
	int atk_precast_audio_preset = -1;
	int atk_hit_audio_preset = -1;
	float cast_time = 1.f; // animation time
	float cast_point = 1.f; // hit point
	uint phy_def = 0; // physical defense
	uint mag_def = 0; // magic defense
	uint hp_reg = 0; // hp regeneration
	uint mp_reg = 0; // mp regeneration
	uint mov_sp = 100; // move speed
	uint atk_sp = 100; // attack speed

	std::vector<std::pair<std::string, uint>> abilities;
	std::vector<std::tuple<uint, uint, uint, uint>> drop_items; // id, probability, num min, num max

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

struct CharacterPoints
{
	uint ability_points = 1;
	uint attribute_points = 0;
	ushort VIG_PTS = 0;
	ushort MND_PTS = 0;
	ushort STR_PTS = 0;
	ushort DEX_PTS = 0;
	ushort INT_PTS = 0;
	ushort LUK_PTS = 0;
};

/// Reflect ctor
struct cCharacter : Component
{
	/// Reflect requires
	cNodePtr node;
	/// Reflect requires
	cNavAgentPtr nav_agent;
	/// Reflect requires
	cAudioSourcePtr audio_source;

	cArmaturePtr armature = nullptr;

	uint id;
	uint preset_id;
	inline const CharacterPreset& get_preset()
	{
		return CharacterPreset::get(preset_id);
	}

	/// Reflect
	uint faction = 0;
	/// Reflect
	void set_faction(uint _faction);
	/// Reflect
	uint ai_id = 0;

	/// Reflect
	uint lv = 1;
	void set_lv(uint v);
	/// Reflect
	uint exp = 0;
	void set_exp(uint v);
	uint exp_max = 0;
	void set_exp_max(uint v);

	State state = StateNormal;

	/// Reflect
	uint hp = 1000;
	void set_hp(uint v);
	/// Reflect
	uint hp_max = 1000;
	void set_hp_max(uint v);

	/// Reflect
	uint mp = 10;
	void set_mp(uint v);
	/// Reflect
	uint mp_max = 10;
	void set_mp_max(uint v);

	/// Reflect
	ushort VIG = 0;
	void set_VIG(ushort v);
	/// Reflect
	ushort MND = 0;
	void set_MND(ushort v);
	/// Reflect
	ushort STR = 0;
	void set_STR(ushort v);
	/// Reflect
	ushort DEX = 0;
	void set_DEX(ushort v);
	/// Reflect
	ushort INT = 0;
	void set_INT(ushort v);
	/// Reflect
	ushort LUK = 0;
	void set_LUK(ushort v);

	/// Reflect
	uchar atk_type = PhysicalDamage;
	void set_atk_type(uchar v);
	/// Reflect
	uint atk = 10;
	void set_atk(uint v);
	/// Reflect
	uint phy_def = 0;
	void set_phy_def(uint v);
	/// Reflect
	uint mag_def = 0;
	void set_mag_def(uint v);
	/// Reflect
	uint hp_reg = 0;
	void set_hp_reg(uint v);
	/// Reflect
	uint mp_reg = 0;
	void set_mp_reg(uint v);
	/// Reflect
	uint mov_sp = 0;
	void set_mov_sp(uint v);
	/// Reflect
	uint atk_sp = 0;
	void set_atk_sp(uint v);

	ivec2 vision_coord = ivec2(-1);
	uint vision_range = 15;

	std::unique_ptr<CharacterPoints> points;

	Listeners<void(cCharacterPtr character, cCharacterPtr target, DamageType type, uint value)> attack_effects;
	Listeners<void(cCharacterPtr character, cCharacterPtr target, DamageType type, uint value)> injury_effects;
	std::vector<std::unique_ptr<AbilityInstance>>	abilities;
	std::vector<std::unique_ptr<ItemInstance>>		inventory;
	EquipmentInstance								equipments[EquipPart_Count];
	std::vector<std::unique_ptr<BuffInstance>>		buffs;
	std::map<uint, float>							markers;

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

	void inflict_damage(cCharacterPtr target, uint value, DamageType type);
	bool take_damage(uint value, DamageType type); // return true if the damage causes the character die
	void gain_exp(uint v);
	bool gain_item(uint id, uint num);
	bool gain_ability(uint id, uint lv = 0);
	void use_item(ItemInstance* ins);
	void cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target);
	void add_buff(uint id, float time, bool replace = false);
	bool add_marker(uint hash, float time);
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
