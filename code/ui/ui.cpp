#include <flame/graphics/canvas.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/image.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "../control.h"
#include "../entities/character.h"
#include "../entities/ability.h"
#include "../entities/chest.h"
#include "ui.h"

graphics::CanvasPtr canvas = nullptr;
EntityPtr ui_ability_slots[4] = { nullptr, nullptr, nullptr, nullptr };
cElementPtr ui_hp_bar = nullptr;
cElementPtr ui_mp_bar = nullptr;
cTextPtr ui_hp_text = nullptr;
cTextPtr ui_mp_text = nullptr;
cTextPtr ui_tip = nullptr;

int circle_lod(float r)
{
	return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
}

void init_ui()
{
	auto renderer = sRenderer::instance();
	canvas = renderer->canvas;

	if (auto ui = root->find_child("ui"); ui)
	{
		if (auto bottom_bar = ui->find_child("bottom_bar"); bottom_bar)
		{
			if (auto abilities_bar = bottom_bar->find_child("abilities_bar"); abilities_bar)
			{
				for (auto i = 0; i < 4; i++)
				{
					auto e = abilities_bar->find_child("slot" + str(i + 1));
					ui_ability_slots[i] = e;
				}
			}

			ui_hp_bar = bottom_bar->find_child("hp_bar")->element();
			ui_mp_bar = bottom_bar->find_child("mp_bar")->element();
			ui_hp_text = bottom_bar->find_child("hp_text")->get_component_t<cText>();
			ui_mp_text = bottom_bar->find_child("mp_text")->get_component_t<cText>();
		}

		if (auto e = ui->find_child("tip"); e)
			ui_tip = e->get_component_t<cText>();
	}
}

void deinit_ui()
{
}

void update_ui()
{
	if (main_player.character)
	{
		for (auto i = 0; i < 4; i++)
		{
			if (auto e = ui_ability_slots[i]; e)
			{
				if (auto ability = main_player.character->get_ability(i); ability)
				{
					if (auto img = e->get_component_t<cImage>(); img)
						img->set_image_name(ability->icon_name);
					if (auto e2 = e->find_child("mana"); e2)
					{
						if (auto txt = e2->get_component_t<cText>(); txt)
							txt->set_text(wstr(ability->mp));
					}
				}
				else
				{
					if (auto img = e->get_component_t<cImage>(); img)
						img->set_image_name(L"");
					if (auto e2 = e->find_child("mana"); e2)
					{
						if (auto txt = e2->get_component_t<cText>(); txt)
							txt->set_text(L"");
					}
				}

				if (ui_hp_bar)
					ui_hp_bar->set_scl(vec2((float)main_player.character->hp / (float)main_player.character->hp_max, 1.f));
				if (ui_mp_bar)
					ui_mp_bar->set_scl(vec2((float)main_player.character->mp / (float)main_player.character->mp_max, 1.f));
				if (ui_hp_text)
					ui_hp_text->set_text(wstr(main_player.character->hp) + L"/" + wstr(main_player.character->hp_max));
				if (ui_mp_text)
					ui_mp_text->set_text(wstr(main_player.character->mp) + L"/" + wstr(main_player.character->mp_max));
			}
		}
	}

	std::wstring tip_str = L"";
	vec3 tip_pos;

	if (main_player.character)
	{
		auto r = main_player.nav_agent->radius + 0.05f;
		auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
		std::vector<vec3> pts(circle_draw.pts.size() * 2);
		auto center = main_player.node->pos;
		center.y += 0.1f;
		for (auto i = 0; i < circle_draw.pts.size(); i++)
		{
			pts[i * 2 + 0] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
			pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
		}
		sRenderer::instance()->draw_primitives("LineList"_h, pts.data(), pts.size(), cvec4(85, 131, 79, 255), true);
	}

	if (main_camera.camera)
	{
		auto camera_x = main_camera.node->x_axis();
		for (auto& character : find_characters_within_camera((FactionFlags)0xffffffff))
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

		if (hovering_character)
		{
			if (auto first_child = hovering_character->entity->children.empty() ? nullptr : hovering_character->entity->children[0].get(); first_child)
			{
				std::vector<CommonDraw> ds;
				for (auto& c : first_child->children)
				{
					if (auto node = c->node(); !node || !AABB_frustum_check(main_camera.camera->frustum, node->bounds))
						continue;
					if (auto mesh = c->get_component_t<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
						ds.emplace_back("mesh"_h, mesh->mesh_res_id, mesh->instance_id);
				}
				sRenderer::instance()->draw_outlines(ds, main_player.character &&
					hovering_character->faction == main_player.character->faction ? cvec4(64, 128, 64, 255) : cvec4(128, 64, 64, 255), 4, "BOX"_h);

				tip_str = s2w(hovering_character->entity->name);
				tip_pos = hovering_character->get_pos() + vec3(0.f, 0.2f, 0.f);
			}
		}
		if (hovering_chest)
		{
			for (auto& c : hovering_chest->entity->get_all_children())
			{
				if (auto node = c->node(); !node || !AABB_frustum_check(main_camera.camera->frustum, node->bounds))
					continue;
				if (auto mesh = c->get_component_t<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
				{
					CommonDraw d("mesh"_h, mesh->mesh_res_id, mesh->instance_id);
					sRenderer::instance()->draw_outlines({ d }, cvec4(64, 128, 64, 255), 4, "BOX"_h);
				}
			}
		}
	}

	if (select_distance > 0.f)
	{
		if (select_range > 0.f)
		{
			auto r = select_range;
			auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
			auto center = hovering_pos;
			std::vector<vec3> pts(circle_draw.pts.size() * 3);
			for (auto i = 0; i < circle_draw.pts.size(); i++)
			{
				pts[i * 2 + 0] = center;
				pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
				pts[i * 2 + 2] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
			}

			sRenderer::instance()->draw_primitives("TriangleList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 100), true);
		}
		else
		{
			auto r = select_distance;
			auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
			auto center = main_player.node->pos;
			std::vector<vec3> pts(circle_draw.pts.size() * 3);
			for (auto i = 0; i < circle_draw.pts.size(); i++)
			{
				pts[i * 3 + 0] = center;
				pts[i * 3 + 1] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
				pts[i * 3 + 2] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
			}

			sRenderer::instance()->draw_primitives("TriangleList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 100), true);
		}
	}

	if (ui_tip)
	{
		if (tip_str.empty())
			ui_tip->entity->set_enable(false);
		else
		{
			if (main_camera.camera)
			{
				ui_tip->entity->set_enable(true);
				ui_tip->set_text(tip_str);
				ui_tip->element->set_pos(main_camera.camera->world_to_screen(tip_pos));
			}
			else
				ui_tip->entity->set_enable(false);
		}
	}
}
