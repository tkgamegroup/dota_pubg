#include <flame/graphics/window.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

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
	}
}main_player;

void cMain::start()
{
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_nav_mesh();
		return false;
	});
	main_player.init(entity->find_child("main_player"));
}

void cMain::update()
{
	auto input = sInput::instance();
	if (input->mpressed(Mouse_Left))
	{
		vec3 p;
		sRenderer::instance()->pick_up(input->mpos, &p);
		printf("%s\n", str(p).c_str());
		main_player.nav->set_target(p);
	}
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
