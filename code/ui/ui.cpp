#include <flame/graphics/canvas.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "../entities/character.h"
#include "ui.h"

static graphics::CanvasPtr canvas = nullptr;

void init_ui()
{
	if (!canvas)
	{
		auto renderer = sRenderer::instance();
		if (renderer->use_window_targets)
			canvas = graphics::Canvas::create(renderer->window);
		else
			canvas = graphics::Canvas::create(renderer->window, renderer->iv_tars);
		canvas->clear_framebuffer = false;
	}
}

void deinit_ui()
{
	if (canvas)
	{
		delete canvas;
		canvas = nullptr;
	}
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
}
