#pragma once

#include "main.h"
#include "ability.h"
#include "item.h"
#include "buff.h"

struct CharacterCommand
{
	uint type;
	cCharacterPtr character;

	CharacterCommand(uint type, cCharacterPtr character);
	virtual ~CharacterCommand() {}
	virtual void update() = 0;
};

struct CharacterCommandIdle : CharacterCommand
{
	CharacterCommandIdle(cCharacterPtr character);

	void update() override;
};

struct CharacterCommandMoveTo : CharacterCommand
{
	vec3 location;

	CharacterCommandMoveTo(cCharacterPtr character, const vec3& _location);

	void update() override;
};

struct CharacterCommandAttackTarget : CharacterCommand
{
	Tracker<cCharacterPtr> target;
	cCharacterPtr precasted_target = nullptr;

	CharacterCommandAttackTarget(cCharacterPtr character, cCharacterPtr _target);

	void update() override;
};

struct CharacterCommandAttackLocation : CharacterCommand
{
	Tracker<cCharacterPtr> target;
	cCharacterPtr precasted_target = nullptr;
	vec3 location;

	CharacterCommandAttackLocation(cCharacterPtr character, const vec3& _location);

	void update() override;
};

struct CharacterCommandPickUp : CharacterCommand
{
	Tracker<cChestPtr> target;

	CharacterCommandPickUp(cCharacterPtr character, cChestPtr _target);

	void update() override;
};

struct CharacterCommandCastAbility : CharacterCommand
{
	AbilityInstance* ins;

	CharacterCommandCastAbility(cCharacterPtr character, AbilityInstance* ins);

	void update() override;
};

struct CharacterCommandCastAbilityToLocation : CharacterCommand
{
	vec3 location;
	AbilityInstance* ins;

	CharacterCommandCastAbilityToLocation(cCharacterPtr character, AbilityInstance* ins, const vec3& _location);

	void update() override;
};

struct CharacterCommandCastAbilityToTarget : CharacterCommand
{
	Tracker<cCharacterPtr> target;
	AbilityInstance* ins;

	CharacterCommandCastAbilityToTarget(cCharacterPtr character, AbilityInstance* ins, cCharacterPtr _target);

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
	uint hp = 100;
	uint mp = 100;
	uint atk = 0;
	float atk_distance = 1.5f;
	float atk_interval = 2.f; // attack interval
	float atk_time = 1.5f; // animation time
	float atk_point = 1.f; // hit point
	int atk_projectile_preset = -1;
	float cast_time = 1.f; // animation time
	float cast_point = 1.f; // hit point
	uint phy_def = 0; // physical defense
	uint mag_def = 0; // magic defense
	uint hp_reg = 0; // hp regeneration
	uint mp_reg = 0; // mp regeneration
	uint mov_sp = 100; // move speed
	uint atk_sp = 100; // attack speed

	std::vector<std::pair<std::string, uint>>		abilities;
	std::vector<std::string>						talents;
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

extern std::vector<cCharacterPtr> characters;
extern std::map<uint, std::vector<cCharacterPtr>> factions;

// Reflect ctor
struct cCharacter : Component
{
	// Reflect requires
	cNodePtr node;
	// Reflect requires
	cNavAgentPtr nav_agent;
	// Reflect requires
	cAudioSourcePtr audio_source;
	// Reflect auto_requires
	cObjectPtr object;

	cArmaturePtr armature = nullptr;

	const CharacterPreset* preset = nullptr;

	// Reflect
	uint faction = 0;
	// Reflect
	void set_faction(uint _faction);
	// Reflect
	uint ai_id = 0;

	// Reflect
	uint lv = 1;
	void set_lv(uint v);
	// Reflect
	uint exp = 0;
	void set_exp(uint v);
	uint exp_max = 0;
	void set_exp_max(uint v);

	State state = StateNormal;

	// Reflect
	uint hp = 1000;
	void set_hp(uint v);
	// Reflect
	uint hp_max = 1000;
	void set_hp_max(uint v);

	// Reflect
	uint mp = 10;
	void set_mp(uint v);
	// Reflect
	uint mp_max = 10;
	void set_mp_max(uint v);

	// Reflect
	uchar atk_type = PhysicalDamage;
	void set_atk_type(uchar v);
	// Reflect
	uint atk = 10;
	void set_atk(uint v);
	// Reflect
	uint phy_def = 0;
	void set_phy_def(uint v);
	// Reflect
	uint mag_def = 0;
	void set_mag_def(uint v);
	// Reflect
	uint hp_reg = 0;
	void set_hp_reg(uint v);
	// Reflect
	uint mp_reg = 0;
	void set_mp_reg(uint v);
	// Reflect
	uint mov_sp = 0;
	void set_mov_sp(uint v);
	// Reflect
	uint atk_sp = 0;
	void set_atk_sp(uint v);

	ivec2 vision_coord = ivec2(-1);
	uint vision_range = 20;

	Listeners<void(cCharacterPtr character, cCharacterPtr target, DamageType type, uint value)> attack_effects;
	Listeners<void(cCharacterPtr character, cCharacterPtr target, DamageType type, uint value)> injury_effects;
	std::vector<std::unique_ptr<ItemInstance>>		inventory;
	std::vector<std::unique_ptr<AbilityInstance>>	abilities;
	std::vector<uint>								talents;
	std::vector<std::unique_ptr<BuffInstance>>		buffs;
	std::map<uint, std::pair<float, uint>>			markers;
	uint ability_points = 0;

	int dead_flag = 0;
	bool stats_dirty = true;
	std::unique_ptr<CharacterCommand> command;
	Action action = ActionNone;
	float move_speed = 1.f;
	float attack_speed = 1.f;
	float cast_speed = 1.f;
	float regeneration_timer = 1.f;
	float search_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_hit_timer = 0.f;
	float attack_timer = 0.f;
	float cast_timer = 0.f;

	~cCharacter();
	void on_init() override;

	void inflict_damage(cCharacterPtr target, DamageType type, uint value);
	bool take_damage(DamageType type, uint value); // return true if the damage causes the character die
	void restore_hp(uint value);
	void restore_mp(uint value);
	void gain_exp(uint v);
	bool gain_item(uint id, uint num);
	bool gain_ability(uint id, uint lv = 0);
	bool gain_talent(uint id);
	void use_item(ItemInstance* ins);
	void cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target);
	void add_buff(uint id, float time, bool replace = false);
	bool add_marker(uint hash, float time);
	void die();

	bool process_approach(const vec3& target, float dist = 0.f, float ang = 0.f);
	void process_attack_target(cCharacterPtr target);
	void process_cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target);

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

void init_characters();
