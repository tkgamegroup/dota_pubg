#include "projectile.h"
#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

static void* ev_delete_projectiles;
static std::vector<cProjectilePtr> dead_projectiles;

void cProjectile::set_target(cCharacterPtr character)
{
	if (target == character)
		return;
	if (target)
		target->entity->message_listeners.remove((uint)this);
	target = character;
	if (target)
	{
		target->entity->message_listeners.add([this](uint h, void*, void*) {
			if (h == "destroyed"_h)
				target = nullptr;
		}, (uint)this);
	}
}

void cProjectile::die()
{
	set_target(nullptr);
	if (!ev_delete_projectiles)
	{
		ev_delete_projectiles = add_event([]() {
			for (auto p : dead_projectiles)
				p->entity->remove_from_parent();
			dead_projectiles.clear();
			ev_delete_projectiles = nullptr;
			return false;
		});
	}
	dead_projectiles.push_back(this);
}

void cProjectile::start()
{
}

void cProjectile::update()
{
	if (on_update)
		on_update(this);

	if (use_target)
	{
		if (!target)
		{
			if (on_end)
				on_end(node->pos, nullptr);
			die();
			return;
		}
		location = target->node->pos + vec3(0.f, target->nav_agent->height * 0.5f, 0.f);
	}

	auto sp = speed * delta_time;
	auto self_pos = node->pos;
	if (distance(self_pos, location) < sp)
	{
		if (on_end)
			on_end(node->pos, target);
		die();
		return;
	}

	node->add_pos(normalize(location - self_pos) * sp);
	node->look_at(location);

	if (collide_radius > 0.f && on_collide)
	{
		for (auto c : find_characters(node->pos, collide_radius, collide_faction))
			on_collide(c);
	}
}

struct cProjectileCreate : cProjectile::Create
{
	cProjectilePtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cProjectile;
	}
}cProjectile_create;
cProjectile::Create& cProjectile::create = cProjectile_create;
