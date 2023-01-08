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
	for (auto& section : parse_ini_file(Path::get(L"assets\\projectiles.ini")).sections)
	{
		auto& preset = projectile_presets.emplace_back();
		preset.id = projectile_presets.size() - 1;
		preset.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "path")
				preset.path = e.values[0];
		}
	}
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

std::vector<cProjectilePtr> projectiles;
std::vector<cProjectilePtr> dead_projectiles;

cProjectile::~cProjectile()
{
	std::erase_if(projectiles, [this](const auto& i) {
		return i == this;
	});
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
