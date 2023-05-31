#pragma once

#include "../head.h"
#include "../command.h"

enum CharacterAction
{
	CharacterActionNone,
	CharacterActionMove,
	CharacterActionAttack,
	CharacterActionCast
};

// Reflect
enum CharacterAttribute
{
	CharacterAttributeHpMax,
	CharacterAttributeMpMax,
	CharacterAttributeAtk,
	CharacterAttributePhyDef,
	CharacterAttributeMagDef,
	CharacterAttributeHpReg,
	CharacterAttributeMpReg,
	CharacterAttributeMovSp,
	CharacterAttributeAtkSp
};

extern std::vector<cCharacterPtr> characters;
extern std::unordered_map<uint, std::vector<cCharacterPtr>> factions;
extern std::vector<cCharacterPtr> dead_characters;
extern bool removing_dead_characters;

struct InitStats
{
	uint hp_max;
	uint mp_max;
	DamageType atk_type;
	uint atk;
	uint phy_def;
	uint mag_def;
	uint hp_reg;
	uint mp_reg;
	uint mov_sp;
	uint atk_sp;
};

// Reflect ctor
struct cCharacter : Component
{
	enum Command
	{
		CommandIdle,
		CommandMoveTo,
		CommandAttackTarget,
		CommandAttackLocation,
		CommandHold,
		CommandInteract,
		CommandCastAbility,
		CommandCastAbilityToLocation,
		CommandCastAbilityToTarget
	};

	struct DropItem
	{
		uint id;
		uint probability;
		uint num_min;
		uint num_max;
	};

	// Reflect requires
	cNodePtr node;
	// Reflect auto_requires
	cObjectPtr object;

	cNavAgentPtr nav_agent = nullptr;
	cNavObstaclePtr nav_obstacle = nullptr;
	cArmaturePtr armature = nullptr;
	cAudioSourcePtr audio_source = nullptr;

	float get_radius();
	float get_height();
	vec3 get_pos(float height_factor = 1.f);

	// Reflect
	FactionFlags faction = FactionCreep;
	// Reflect
	void set_faction(FactionFlags _faction);

	// Reflect
	uint lv = 1;
	void set_lv(uint v);
	// Reflect
	uint exp = 0;
	void set_exp(uint v);
	uint exp_max = 0;
	void set_exp_max(uint v);

	// Reflect
	CharacterStateFlags state = CharacterStateNormal;

	// Reflect
	uint hp = 100;
	void set_hp(uint v);
	// Reflect
	uint hp_max = 100;
	void set_hp_max(uint v);

	// Reflect
	uint mp = 100;
	void set_mp(uint v);
	// Reflect
	uint mp_max = 100;
	void set_mp_max(uint v);

	// Reflect
	uint exp_base = 0;
	// Reflect
	DamageType atk_type = PhysicalDamage;
	// Reflect
	void set_atk_type(DamageType v);
	// Reflect
	uint atk = 10;
	// Reflect
	void set_atk(uint v);
	// Reflect
	float atk_distance = 1.5f;
	// Reflect
	void set_atk_distance(float v);
	// Reflect
	float atk_interval = 2.f;
	// Reflect
	float atk_time = 1.5f;
	// Reflect
	float atk_point = 1.f;
	// Reflect
	std::filesystem::path atk_projectile;
	// Reflect
	uint phy_def = 0;
	// Reflect
	void set_phy_def(uint v);
	// Reflect
	uint mag_def = 0;
	// Reflect
	void set_mag_def(uint v);
	// Reflect
	uint hp_reg = 0;
	// Reflect
	void set_hp_reg(uint v);
	// Reflect
	uint mp_reg = 0;
	// Reflect
	void set_mp_reg(uint v);
	// Reflect
	uint mov_sp = 100;
	// Reflect
	void set_mov_sp(uint v);
	// Reflect
	uint atk_sp = 100;
	// Reflect
	void set_atk_sp(uint v);

	std::vector<DropItem>		drop_items;

	std::filesystem::path		move_sound_path;
	std::filesystem::path		attack_precast_sound_path;
	std::filesystem::path		attack_hit_sound_path;

	ivec2 vision_coord = ivec2(-1);
	uint vision_range = 20;

	char items_idx = -1;
	char abilities_idx = -1;
	inline cAbilityPtr get_ability(int idx)
	{
		if (abilities_idx != -1)
		{
			auto e = entity->children[abilities_idx].get();
			if (e->children.size() > idx)
				return e->children[idx]->get_component_t<cAbility>();
		}
		return nullptr;
	}
	//char talents_idx = -1;
	char buffs_idx = -1;
	char attack_effects_idx = -1;
	std::map<uint, std::pair<float, uint>>	markers;
	uint ability_points = 0;

	bool dead = false;
	bool stats_dirty = true;
	InitStats init_stats;
	Command command = CommandIdle;
	Tracker target; // character, chest, shop, etc
	vec3	target_location;
	voidptr target_obj; // ability, item, etc
	CharacterAction action = CharacterActionNone;
	float move_speed = 1.f;
	float attack_speed = 1.f;
	float cast_speed = 1.f;
	float regeneration_timer = 1.f;
	float search_timer = 0.f;

	float attack_interval_timer = 0.f;
	float pre_action_time = 0.f;
	float total_action_time = 0.f;
	float action_timer = -1.f;

	Listeners<void(CharacterMessage msg, sVariant p0, sVariant p1, sVariant p2, sVariant p3)> message_listeners;

	~cCharacter();

	void on_init() override;
	void start() override;
	void update() override;

	void die(uint type);

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

	void reset_cmd();
	void cmd_idle();
	void cmd_move_to(const vec3& location);
	void cmd_attack_target(cCharacterPtr target);
	void cmd_attack_location(const vec3& location);
	void cmd_hold();
	void cmd_interact(ComponentPtr target);
	void cmd_cast_ability(cAbilityPtr ability);
	void cmd_cast_ability_to_location(cAbilityPtr ability, const vec3& location);
	void cmd_cast_ability_to_target(cAbilityPtr ability, cCharacterPtr target);

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

