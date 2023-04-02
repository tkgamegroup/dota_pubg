#include <flame/graphics/canvas.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/image.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "../entities/character.h"
#include "../entities/ability.h"
#include "ui.h"

graphics::CanvasPtr canvas = nullptr;
EntityPtr ui_ability_slots[4];

void init_ui()
{
	canvas = sRenderer::instance()->canvas;

	if (auto ui = root->find_child("ui"); ui)
	{
		if (auto bar = ui->find_child("abilities_bar"); bar)
		{
			for (auto i = 0; i < 4; i++)
			{
				auto e = bar->find_child("slot" + str(i + 1));
				ui_ability_slots[i] = e;
			}
		}
	}
}

void deinit_ui()
{
}

void update_ui()
{
	if (main_camera.camera)
	{
		auto camera_x = main_camera.node->x_axis();
		for (auto& character : find_characters_within_camera())
		{
			auto radius = character->get_radius();
			auto height = character->get_height();
			auto pos = character->node->pos;
			auto p0 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.1f, 0.f) - camera_x * radius);
			auto p1 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.1f, 0.f) + camera_x * radius);
			auto p2 = main_camera.camera->world_to_screen(pos + vec3(0.f, height, 0.f) + camera_x * radius);
			auto w = p1.x - p0.x;
			auto h = p2.y - p0.y;
			if (w > 0.f && h > 0.f)
				canvas->add_rect_filled(p0, p0 + vec2((float)character->hp / (float)character->hp_max * w, h), cvec4(80, 160, 85, 255));
		}
	}

	if (main_player.character)
	{
		for (auto i = 0; i < 4; i++)
		{
			if (auto e = ui_ability_slots[i]; e)
			{
				if (auto ability = main_player.character->get_ability(i); ability)
					e->get_component_t<cImage>()->set_image_name(ability->icon_name);
				else
					e->get_component_t<cImage>()->set_image_name(L"");
			}
		}
	}
}
