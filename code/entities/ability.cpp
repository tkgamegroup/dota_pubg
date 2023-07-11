#include "../game.h"+
#include "ability.h"
#include "character.h"
#include "buff.h"
#include "projectile.h"
#include "effect.h"
#include "collider.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/particle_system.h>

// Reflect ctor dtor
struct EXPORT AbilityFunc_add_attribute : PassiveAbilityFunc
{
	~AbilityFunc_add_attribute() {}

	// Reflect
	std::vector<std::pair<CharacterAttribute, int>> attribute_changes;

	void exec(cAbilityPtr ability, cCharacterPtr character) override
	{
		for (auto& entry : attribute_changes)
		{
			switch (entry.first)
			{
			case CharacterAttributeHpMax:
				character->hp_max += entry.second;
				break;
			}
		}
	}
};

// Reflect ctor dtor
struct EXPORT AbilityFunc_thorwer : ActiveAbilityFunc
{
	~AbilityFunc_thorwer() {}

	// Reflect
	std::string effect_name;
	// Reflect
	float inner_radius = 2.f;
	// Reflect
	float outer_radius = 3.f;
	// Reflect
	float length = 2.f;
	// Reflect
	float angle = 22.5f;
	// Reflect
	float speed = 8.f;
	// Reflect
	uint damage = 50;

	void exec(cAbilityPtr ability, cCharacterPtr character, const vec3& location, cCharacterPtr target) override
	{
		if (effect_name.empty())
		{
			printf("Ability Thorwer: effect_name is empty\n");
			return;
		}

		auto distance = ability->distance * (1.f/*distance modifier*/);
		auto duration = distance / speed;

		auto info = effect_infos.find(effect_name);
		if (!info)
		{
			printf("Ability Thorwer: cannot find effect\n");
			return;
		}
		auto effect = add_effect(info, character->get_pos(0.5f), angleAxis(radians(angle_xz(character->node->pos, location)), vec3(0.f, 1.f, 0.f)), duration);
		if (!effect)
		{
			printf("Ability Thorwer: cannot find prefab or prefab is missing cEffect\n");
			return;
		}

		auto e = effect->entity;
		if (auto collider = e->get_component<cSectorCollider>(); collider)
		{
			collider->faction = ~character->faction;
			collider->inner_radius = inner_radius;
			collider->outer_radius = outer_radius;
			collider->length = length;
			collider->angle = angle;
			collider->speed = speed;
			collider->delay = 0.f;
			collider->duration = duration;
			collider->callbacks.add([this, character](cCharacterPtr target, bool enter_or_exit) {
				character->inflict_damage(target, MagicalDamage, damage);
			});
		}
		if (auto e2 = !e->children.empty() ? e->children[0].get() : nullptr; e2)
		{
			if (auto ps = e2->get_component<cParticleSystem>(); ps)
			{
				e2->get_component<cNode>()->set_pos(vec3(-inner_radius, 0.f, 0.f));
				ps->particle_life_time = duration;
				ps->particle_speed = speed;
				ps->emitt_duration = length / speed;
				ps->emitt_offset = inner_radius;
				ps->emitt_angle = angle;
			}
		}
	}
};

std::vector<Talent> talents;

struct cAbilityCreate : cAbility::Create
{
	cAbilityPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cAbility;
	}
}cAbility_create;
cAbility::Create& cAbility::create = cAbility_create;
