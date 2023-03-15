#pragma once

#include "../head.h"
#include "../command.h"

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

	CharacterCommandAttackTarget(cCharacterPtr character, cCharacterPtr _target);

	void update() override;
};

struct CharacterCommandAttackLocation : CharacterCommand
{
	Tracker<cCharacterPtr> target;
	vec3 location;

	CharacterCommandAttackLocation(cCharacterPtr character, const vec3& _location);

	void update() override;
};

struct CharacterCommandHold : CharacterCommand
{
	Tracker<cCharacterPtr> target;

	CharacterCommandHold(cCharacterPtr character);

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
	cAbilityPtr ability;

	CharacterCommandCastAbility(cCharacterPtr character, cAbilityPtr ability);

	void update() override;
};

struct CharacterCommandCastAbilityToLocation : CharacterCommand
{
	vec3 location;
	cAbilityPtr ability;

	CharacterCommandCastAbilityToLocation(cCharacterPtr character, cAbilityPtr ability, const vec3& _location);

	void update() override;
};

struct CharacterCommandCastAbilityToTarget : CharacterCommand
{
	Tracker<cCharacterPtr> target;
	cAbilityPtr ability;

	CharacterCommandCastAbilityToTarget(cCharacterPtr character, cAbilityPtr ability, cCharacterPtr _target);

	void update() override;
};

enum Action
{
	ActionNone,
	ActionMove,
	ActionAttack,
	ActionCast
};

extern std::vector<cCharacterPtr> characters;
extern std::unordered_map<uint, std::vector<cCharacterPtr>> factions;
extern std::vector<cCharacterPtr> dead_characters;
extern bool removing_dead_characters;

// Reflect ctor
struct cCharacter : Component
{
	struct DropItem
	{
		uint id;
		uint probability;
		uint num_min;
		uint num_max;
	};

	// Reflect requires
	cNodePtr node;
	// Reflect requires
	cNavAgentPtr nav_agent;
	// Reflect requires
	cAudioSourcePtr audio_source;
	// Reflect auto_requires
	cObjectPtr object;

	cArmaturePtr armature = nullptr;

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

	CharacterState state = CharacterStateNormal;

	// Reflect
	uint hp = 1;
	void set_hp(uint v);
	// Reflect
	uint hp_max = 1;
	void set_hp_max(uint v);

	// Reflect
	uint mp = 1;
	void set_mp(uint v);
	// Reflect
	uint mp_max = 1;
	void set_mp_max(uint v);

	// Reflect
	uint exp_base = 0;
	// Reflect
	uchar atk_type = PhysicalDamage;
	void set_atk_type(uchar v);
	// Reflect
	uint atk = 10;
	void set_atk(uint v);
	// Reflect
	float atk_distance = 1.5f;
	// Reflect
	float atk_interval = 2.f;
	// Reflect
	float atk_time = 1.5f;
	// Reflect
	float atk_point = 1.f;
	// Reflect
	std::filesystem::path atk_projectile;
	uint atk_projectile_id = 0;
	// Reflect
	float cast_time = 1.f;
	// Reflect
	float cast_point = 1.f;
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

	std::vector<DropItem>		drop_items;

	std::filesystem::path		move_sound_path;
	std::filesystem::path		attack_precast_sound_path;
	std::filesystem::path		attack_hit_sound_path;

	ivec2 vision_coord = ivec2(-1);
	uint vision_range = 20;

	std::vector<CommandList>				attack_effects;
	//std::vector<cItemPtr>					inventory;
	//std::vector<cAbilityPtr>				abilities;
	//std::vector<cTalentPtr>				talents;
	//std::vector<cBuffPtr>					buffs;
	std::map<uint, std::pair<float, uint>>	markers;
	uint ability_points = 0;

	bool dead = false;
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

	Listeners<void(CharacterMessage msg, sVariant p0, sVariant p1, sVariant p2, sVariant p3)> message_listeners;

	~cCharacter();

	void on_init() override;
	void start() override;
	void update() override;

	void die();

	void inflict_damage(cCharacterPtr target, DamageType type, uint value);
	bool take_damage(DamageType type, uint value); // return true if the damage causes the character die
	void restore_hp(uint value);
	void restore_mp(uint value);
	void gain_exp(uint v);
	bool gain_item(uint id, uint num);
	bool gain_ability(uint id, uint lv = 0);
	bool gain_talent(uint id);
	void use_item(cItemPtr item);
	void cast_ability(cAbilityPtr ability, const vec3& location, cCharacterPtr target);
	void add_buff(uint id, float time, uint lv = 1, bool replace = false);
	bool add_marker(uint hash, float time);

	bool process_approach(const vec3& target, float dist = 0.f, float ang = 0.f);
	void process_attack_target(cCharacterPtr target, bool chase_target = true);
	void process_cast_ability(cAbilityPtr ability, const vec3& location, cCharacterPtr target);

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

