#include "spwaner.h"

#include <flame/universe/entity.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

void cSpwaner::set_prefab_path(const std::filesystem::path& path)
{
	if (prefab_path == path)
		return;
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
			auto a = 2.f * prefab_radius / (nav_agent->radius + prefab_radius);
			for (auto ang = 0.f; ang < 360.f; ang += a)
			{

			}
			spwan_timer = spwan_interval;
		}
	}
}
