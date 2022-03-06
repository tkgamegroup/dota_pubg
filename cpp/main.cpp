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

cNodePtr camera_node;
cPlayerPtr main_player;

void cMain::start()
{
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_nav_mesh();
		return false;
	});
	camera_node = entity->find_child("Camera")->get_component_i<cNode>(0);
	main_player = entity->find_child("main_player")->get_component_t<cPlayer>();
}

void cMain::update()
{
	auto input = sInput::instance();
	if (input->mpressed(Mouse_Left))
	{
		vec3 p;
		auto obj = sRenderer::instance()->pick_up(input->mpos, &p);
		if (obj)
			main_player->character->nav->set_target(p);
	}

	camera_node->set_eul(vec3(0.f, -camera_angle, 0.f));
	camera_node->set_pos(main_player->character->node->g_pos + camera_length * camera_node->g_rot[2]);
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
