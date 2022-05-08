#include <flame/graphics/window.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

#include "main.h"
#include "character.h"
#include "player.h"

struct MainCamera
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cCameraPtr camera = nullptr;

	void init(EntityPtr e)
	{
		entity = e;
		if (e)
		{
			node = e->get_component_i<cNode>(0);
			camera = e->get_component_t<cCamera>();
		}
	}
}main_camera;

struct MainPlayer
{
	EntityPtr entity = nullptr;
	cNodePtr node = nullptr;
	cNavAgentPtr nav_agent = nullptr;
	cCharacterPtr character = nullptr;
	cPlayerPtr player = nullptr;

	void init(EntityPtr e)
	{
		entity = e;
		if (e)
		{
			node = e->get_component_i<cNode>(0);
			nav_agent = e->get_component_t<cNavAgent>();
			character = e->get_component_t<cCharacter>();
			player = e->get_component_t<cPlayer>();
		}
	}
}main_player;

void cMain::start()
{
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_nav_mesh();
		return false;
	});

	main_camera.init(entity->find_child("Camera"));
	main_player.init(entity->find_child("main_player"));
}

void cMain::update()
{
	auto input = sInput::instance();
	if (input->mpressed(Mouse_Right))
	{
		if (main_player.nav_agent)
		{
			vec3 p;
			auto obj = sRenderer::instance()->pick_up(input->mpos, &p);
			if (obj)
			{
				static EntityPtr e_arrow = nullptr;
				if (!e_arrow)
				{
					e_arrow = Entity::create();
					e_arrow->load(L"assets/arrow.prefab");
					entity->add_child(e_arrow);
				}
				e_arrow->get_component_i<cNode>(0)->set_pos(p);
				main_player.nav_agent->set_target(p);
			}
		}
	}

	if (main_camera.node)
	{
		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
		main_camera.node->set_pos(mix(main_camera.node->pos, main_player.node->g_pos + camera_length * main_camera.node->g_rot[2], 0.1f));
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
	cCharacter::create((EntityPtr)INVALID_POINTER);
	cPlayer::create((EntityPtr)INVALID_POINTER);
	return nullptr;
}
