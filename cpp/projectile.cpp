#include "projectile.h"
#include "character.h"
#include "network.h"

#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

std::vector<ProjectilePreset> projectile_presets;

void load_projectile_presets()
{
	{
		auto& preset = projectile_presets.emplace_back();
		preset.id = projectile_presets.size() - 1;
		preset.path = L"assets\\models\\fireball.prefab";
		preset.name = "Fire Ball";
		preset.update = [](cProjectilePtr pt) {
			pt->node->scl += 2.5f * delta_time;
			pt->node->set_scl(pt->node->scl);
		};
	}
}

int ProjectilePreset::find(const std::string& name)
{
	if (projectile_presets.empty())
		load_projectile_presets();
	for (auto i = 0; i < projectile_presets.size(); i++)
	{
		if (projectile_presets[i].name == name)
			return i;
	}
	return -1;
}

const ProjectilePreset& ProjectilePreset::get(uint id)
{
	if (projectile_presets.empty())
		load_projectile_presets();
	return projectile_presets[id];
}

void cProjectile::start()
{
}

void cProjectile::update()
{
	auto& preset = get_preset();
	if (preset.update)
		preset.update(this);

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
