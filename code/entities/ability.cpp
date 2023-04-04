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
struct EXPORT AbilityFunc_fire_breath : ActiveAbilityFunc
{
	~AbilityFunc_fire_breath() {}

	// Reflect
	uint damage = 50;

	void exec(cAbilityPtr ability, cCharacterPtr character, const vec3& location, cCharacterPtr target) override
	{
		auto effect = add_effect(L"assets\\effects\\flame_thrower.prefab", character->get_pos(0.5f), angleAxis(angle_xz(character->node->pos, location), vec3(0.f, 1.f, 0.f)), 1.55f);
		if (auto collider = effect->entity->get_component_t<cSectorCollider>(); collider)
		{
			collider->faction = ~character->faction;
			collider->callbacks.add([this, character](cCharacterPtr target, bool enter_or_exit) {
				character->inflict_damage(target, MagicDamage, damage);
			});
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
