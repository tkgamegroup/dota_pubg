#pragma once

#include "main.h"

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

enum Action
{
	ActionNone,
	ActionMove,
	ActionAttack
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
	uint lv = 1;
	/// Reflect
	uint exp = 0;
	/// Reflect
	uint exp_max = 0;
	/// Reflect
	uint hp = 100;
	/// Reflect
	uint hp_max = 100;
	/// Reflect
	uint mp = 100;
	/// Reflect
	uint mp_max = 100;
	/// Reflect
	uint STR = 10;
	/// Reflect
	uint AGI = 10;
	/// Reflect
	uint INT = 10;
	/// Reflect
	uint atk = 10;
	/// Reflect
	float atk_distance = 1.5f;
	/// Reflect
	float atk_interval = 2.f; // time
	/// Reflect
	float atk_precast = 0.5f; // 0-1, of the atk_interval
	/// Reflect
	std::filesystem::path atk_projectile_name;
	/// Reflect
	void set_atk_projectile_name(const std::filesystem::path& name);
	/// Reflect
	uint armor = 0;
	/// Reflect
	uint mov_sp = 100;
	/// Reflect
	uint atk_sp = 100;
	/// Reflect
	uint faction = 0;
	/// Reflect
	uint ai_id = 0;

	int inventory[6] = { -1, -1, -1, -1, -1, -1 };

	bool dead = false;
	bool stats_dirty = true;
	std::unique_ptr<Command> command;
	Action action = ActionNone;
	float move_speed = 1.f;
	float attack_speed = 1.f;
	float search_timer = 0.f;
	float attack_interval_timer = 0.f;
	float attack_timer = 0.f;
	EntityPtr atk_projectile = nullptr;

	~cCharacter();
	void on_init() override;

	std::vector<cCharacterPtr> find_enemies(float radius = 0.f, bool ignore_timer = true, bool sort = false);

	void inflict_damage(cCharacterPtr target, uint value);
	bool take_damage(uint value); // return true if the damage causes the character die
	void manipulate_item(int idx0, int idx1, int item_id);
	void level_up();
	void die();

	void move_to(const vec3& target);
	void attack_target(cCharacterPtr target);

	void start() override;
	void update() override;

	struct Create
	{
		virtual cCharacterPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	EXPORT static Create& create;
};
