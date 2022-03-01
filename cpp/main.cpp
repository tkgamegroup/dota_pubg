#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

#include "main.h"

struct MainPlayer
{
	EntityPtr e;
	cNodePtr node;
	cNavAgentPtr nav;

	void init(EntityPtr _e)
	{
		e = _e;
		node = e->get_component_i<cNode>(0);
		nav = e->get_component_t<cNavAgent>();
		add_event([this]() {
			nav->set_target(node->g_pos + vec3(1.f, 0.f, 0.f));
			return false;
		}, 1.f);
	}
}main_player;

void cMain::start()
{
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_navmesh();
		return false;
	});
	main_player.init(entity->find_child("main_player"));
}

void cMain::update()
{

}

struct cMainCreate : cMain::Create
{
	cMainPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cMain;
	}
}cMain_create;
cMain::Create& cMain::create = cMain_create;

EXPORT void* cpp_info()
{
	auto uinfo = universe_info();
	cMain::create((EntityPtr)INVALID_POINTER);
	return nullptr;
}
