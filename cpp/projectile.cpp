#include "projectile.h"
#include "character.h"
#include "network.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

std::vector<ProjectilePreset> projectile_presets;

void init_projectiles()
{
}

int ProjectilePreset::find(const std::string& name)
{
	for (auto i = 0; i < projectile_presets.size(); i++)
	{
		if (projectile_presets[i].name == name)
			return i;
	}
	return -1;
}

const ProjectilePreset& ProjectilePreset::get(uint id)
{
	return projectile_presets[id];
}

void cProjectile::update()
{
	if (preset->update)
		preset->update(this);

	if (use_target)
	{
		if (!target.obj)
		{
			if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
			{
				if (on_end)
					on_end(node->pos, nullptr);

				add_event([this]() {
					entity->remove_from_parent();
					return false;
				});
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

			add_event([this]() {
				entity->remove_from_parent();
				return false;
			});
		}
		return;
	}

	node->add_pos(normalize(location - self_pos) * sp);
	node->look_at(location);
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
