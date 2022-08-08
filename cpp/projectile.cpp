#include "projectile.h"
#include "character.h"

#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>

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

void cProjectile::setup(cCharacterPtr target, const std::function<void(cCharacterPtr)>& cb)
{
	set_target(target);
	callback = cb;
}

void cProjectile::die()
{
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
	if (!target)
	{
		if (callback)
			callback(nullptr);
		die();
		return;
	}
	else
	{
		auto pa = node->g_pos;
		auto pb = target->node->g_pos + vec3(0.f, target->height * 0.5f, 0.f);
		if (distance(pa, pb) < speed)
		{
			if (callback)
				callback(target);
			die();
			return;
		}
		node->add_pos(normalize(pb - pa) * speed);
		node->set_qut(quat(inverse(mat3(lookAt(pa, pb, vec3(0.f, 1.f, 0.f))))));
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
