#include "projectile.h"
#include "character.h"

#include <flame/universe/octree.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

std::vector<cProjectilePtr> projectiles;
std::vector<cProjectilePtr> dead_projectiles;
bool removing_dead_projectiles = false;

cProjectile::~cProjectile()
{
	std::erase_if(projectiles, [this](const auto& i) {
		return i == this;
	});
	if (dead && !removing_dead_projectiles)
	{
		std::erase_if(dead_projectiles, [this](const auto& i) {
			return i == this;
		});
	}
}

void cProjectile::update()
{
	if (dead)
		return;

	if (use_target)
	{
		if (!target.obj)
		{
			if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
			{
				if (on_end)
					on_end(node->pos, nullptr);

				die();
			}
			return;
		}
		location = target.obj->node->pos + vec3(0.f, target.obj->nav_agent->height * 0.5f, 0.f);
	}

	auto sp = speed * delta_time;
	auto self_pos = node->pos;
	if (distance(self_pos, location) < sp)
	{
		if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		{
			if (on_end)
				on_end(node->pos, target.obj);

			die();
		}
		return;
	}

	node->add_pos(normalize(location - self_pos) * sp);
	node->look_at(location);
}

void cProjectile::die()
{
	if (dead)
		return;

	dead_projectiles.push_back(this);
	dead = true;
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
