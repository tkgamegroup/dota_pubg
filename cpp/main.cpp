#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/window.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

#include "main.h"
#include "character.h"
#include "spwaner.h"

void MainCamera::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->get_component_i<cNode>(0);
		camera = e->get_component_t<cCamera>();
	}
}

void MainPlayer::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->get_component_i<cNode>(0);
		nav_agent = e->get_component_t<cNavAgent>();
		character = e->get_component_t<cCharacter>();
	}
}

MainCamera main_camera;
MainPlayer main_player;

std::vector<vec2> site_positions;

void cMain::start()
{
	srand(time(0));
	graphics::gui_set_current();
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_nav_mesh();
		return false;
	});

	main_camera.init(entity->find_child("Camera"));
	main_player.init(entity->find_child("main_player"));

	if (auto e_terrain = entity->find_child("terrain"); e_terrain)
	{
		if (auto terrain = e_terrain->get_component_t<cTerrain>(); terrain)
		{
			if (auto height_map_info_fn = terrain->height_map->filename; !height_map_info_fn.empty())
			{
				height_map_info_fn += L".info";
				std::ifstream file(height_map_info_fn);
				if (file.good())
				{
					LineReader info(file);
					info.read_block("sites:");
					unserialize_text(info, &site_positions);
					file.close();
				}
			}
		}
	}
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
				main_player.character->enter_move_state(p);
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

float random01()
{
	static std::random_device rd;
	static std::mt19937 mt(rd());
	static std::uniform_real_distribution<float> dt;
	return dt(mt);
}

EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cMain::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cSpwaner::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}
