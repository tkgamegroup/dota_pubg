#include "projectile.h"
#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

void cProjectile::start()
{
}

void cProjectile::update()
{
	if (on_update)
		on_update(this);

	if (use_target)
	{
		if (!target.obj)
		{
			if (on_end)
				on_end(node->pos, nullptr);
			remove_projectile(this);
			return;
		}
		location = target.obj->node->pos + vec3(0.f, target.obj->nav_agent->height * 0.5f, 0.f);
	}

	auto sp = speed * delta_time;
	auto self_pos = node->pos;
	if (distance(self_pos, location) < sp)
	{
		if (on_end)
			on_end(node->pos, target.obj);
		remove_projectile(this);
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
