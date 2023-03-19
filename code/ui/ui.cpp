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
	}
}

void deinit_ui()
{
	if (canvas)
		delete canvas;
}

void update_ui()
{
	if (main_camera.camera)
	{
		for (auto& character : find_characters_within_camera())
		{
			auto p = main_camera.camera->world_to_screen(character->node->pos + vec3(0.f, character->nav_agent->height + 0.2f, 0.f));
			if (p.x > 0.f && p.y > 0.f)
			{
				const auto bar_width = 80.f * (character->nav_agent->radius / 0.6f);
				const auto bar_height = 5.f;
				p.x -= bar_width * 0.5f;
				canvas->add_rect_filled(p, p + vec2((float)character->hp / (float)character->hp_max * bar_width, bar_height), cvec4(0, 255, 0, 255));
			}
		}
	}
}
