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

void init_ui()
{
	canvas = sRenderer::instance()->canvas;

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
	}

	root->node()->drawers.add([](DrawData& draw_data) {
		switch (draw_data.pass)
		{
		case PassOutline:
			if (hovering_character)
			{
				if (auto armature = hovering_character->armature; armature && armature->model)
				{
					for (auto& c : armature->entity->children)
					{
						if (auto mesh = c->get_component_t<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
							draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, -1,
								main_player.character && hovering_character->faction == main_player.character->faction ? cvec4(64, 128, 64, 255) : cvec4(128, 64, 64, 255));
					}
				}
			}
			if (hovering_chest)
			{
				for (auto& c : hovering_chest->entity->get_all_children())
				{
					if (auto mesh = c->get_component_t<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
						draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, -1, cvec4(64, 128, 64, 255));
				}
			}
			break;
		case PassPrimitive:
		{
			auto circle_lod = [](float r) {
				return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
			};
			//if (main_player.character)
			//{
			//	auto r = main_player.nav_agent->radius;
			//	auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
			//	std::vector<vec3> pts(circle_draw.pts.size() * 2);
			//	auto center = main_player.node->pos;
			//	for (auto i = 0; i < circle_draw.pts.size(); i++)
			//	{
			//		pts[i * 2 + 0] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
			//		pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
			//	}
			//	draw_data.primitives.emplace_back("LineList"_h, std::move(pts), cvec4(255, 255, 255, 255));
			//}
			if (select_distance > 0.f)
			{
				if (select_angle > 0.f)
				{
					auto r = select_distance;
					auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
					auto center = main_player.node->pos;
					auto dir = hovering_pos - center;
					center -= normalize(dir) * select_start_radius;
					auto ang = angle_xz(dir);
					std::vector<vec3> pts;
					auto i_beg = circle_draw.get_idx(ang - select_angle);
					auto i_end = circle_draw.get_idx(ang + select_angle);
					for (auto i = i_beg; i < i_end; i++)
					{
						auto a = center + vec3(select_start_radius * circle_draw.get_pt(i + 1), 0.f).xzy();
						auto b = center + vec3(select_start_radius * circle_draw.get_pt(i), 0.f).xzy();
						auto c = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
						auto d = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();

						pts.push_back(a);
						pts.push_back(b);
						pts.push_back(d);

						pts.push_back(d);
						pts.push_back(b);
						pts.push_back(c);
					}

					draw_data.primitives.emplace_back("TriangleList"_h, std::move(pts), cvec4(0, 255, 0, 100));
				}
				else if (select_range > 0.f)
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

					draw_data.primitives.emplace_back("TriangleList"_h, std::move(pts), cvec4(0, 255, 0, 100));
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

					draw_data.primitives.emplace_back("TriangleList"_h, std::move(pts), cvec4(0, 255, 0, 100));
				}
			}
		}
			break;
		}
	}, "ui"_h);
}

void deinit_ui()
{
	if (root)
		root->node()->drawers.remove("ui"_h);
}

void update_ui()
{
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
	}

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
}
