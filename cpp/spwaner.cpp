#include "spwaner.h"

#include <flame/universe/entity.h>
#include <flame/universe/world.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

void cSpwaner::set_prefab_path(const std::filesystem::path& path)
{
	if (prefab_path == path)
		return;
	prefab_path = path;
	auto e = Entity::create();
	e->load(prefab_path);
	auto nav = e->get_component_t<cNavAgent>();
	if (!nav)
	{
		delete e;
		return;
	}
	if (prefab)
		delete prefab;
	prefab = e;
	prefab_radius = nav->radius;
}

void cSpwaner::start()
{
	spwan_timer = spwan_interval;
}

void cSpwaner::update()
{
	auto dt = delta_time;
	if (spwan_timer > 0)
		spwan_timer -= dt;

	if (prefab)
	{
		if (spwan_timer <= 0.f)
		{
			auto e = prefab->copy();
			auto p = node->g_pos + node->g_rot[0] * (nav_agent->radius + random01() * 0.5f) + 
				node->g_rot[2] * (random01() * 1.f - 0.5f);
			e->get_component_i<cNode>(0)->set_pos(p);
			root->add_child(e);
			for (auto& cb : callbacks.list)
				cb.first(e);
			spwan_timer = spwan_interval;
		}
	}
}

struct cSpwanerCreate : cSpwaner::Create
{
	cSpwanerPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cSpwaner;
	}
}cSpwaner_create;
cSpwaner::Create& cSpwaner::create = cSpwaner_create;
