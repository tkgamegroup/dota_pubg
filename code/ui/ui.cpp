#include <flame/graphics/canvas.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/text.h>
#include <flame/universe/components/image.h>
#include <flame/universe/components/list.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "../player.h"
#include "../control.h"
#include "../entities/character.h"
#include "../entities/ability.h"
#include "../entities/chest.h"
#include "ui.h"

struct UI_building_window
{
	EntityPtr window = nullptr;
	EntityPtr building_area = nullptr;
	EntityPtr empty_case = nullptr;
	EntityPtr built_up_case = nullptr;

	void init(EntityPtr ui)
	{
		window = ui->find_child("building_window");
		if (window)
		{
			building_area = window->find_child_recursively("building_area");
			empty_case = window->find_child_recursively("empty_case");
			built_up_case = window->find_child_recursively("built_up_case");
		}
	}

}ui_building_window;

struct UI_troop_window
{
	EntityPtr window = nullptr;
	EntityPtr formation_area = nullptr;
	EntityPtr unit_list = nullptr;

	void init(EntityPtr ui)
	{
		window = ui->find_child("troop_window");
		if (window)
		{
			formation_area = window->find_child_recursively("formation_area");
			unit_list = window->find_child_recursively("unit_list");
		}
	}
}ui_troop_window;

EntityPtr ui_route_window = nullptr;

// Reflect ctor dtor
struct EXPORT Action_open_building_window : Action
{
	void exec() override
	{
		if (ui_building_window.window)
			ui_building_window.window->set_enable(!ui_building_window.window->enable);
		if (ui_troop_window.window)
			ui_troop_window.window->set_enable(false);
		if (ui_route_window)
			ui_route_window->set_enable(false);
	}
};

// Reflect ctor dtor
struct EXPORT Action_building_slot_select : Action
{
	// Reflect
	uint index;

	void exec() override
	{
		if (ui_building_window.building_area)
		{
			for (auto& c : ui_building_window.building_area->children)
			{
				if (auto element = c->element(); element)
				{
					auto col = element->frame_col;
					col.a = 0;
					element->set_frame_col(col);
				}
			}

			if (auto element = ui_building_window.building_area->children[index]->element(); element)
			{
				auto col = element->frame_col;
				col.a = 255;
				element->set_frame_col(col);
			}
		}
	}
};

uint selected_unit_index = 0;

// Reflect ctor dtor
struct EXPORT Action_open_troop_window : Action
{
	void exec() override
	{
		if (ui_troop_window.window)
			ui_troop_window.window->set_enable(!ui_troop_window.window->enable);
		if (ui_building_window.window)
			ui_building_window.window->set_enable(false);
		if (ui_route_window)
			ui_route_window->set_enable(false);

		if (ui_troop_window.window->enable)
		{
			selected_unit_index = 0;

			if (ui_troop_window.formation_area)
			{
				if (auto list = ui_troop_window.formation_area->get_component_t<cList>(); list)
				{
					list->set_count(player1.formation.size());
					for (auto i = 0; i < player1.formation.size(); i++)
					{
						if (auto image = ui_troop_window.formation_area->children[i]->get_component_t<cImage>(); image)
							image->set_image_name(player1.formation[i] ? player1.formation[i]->icon_name : L"");
					}
				}
			}

			if (ui_troop_window.unit_list)
			{
				if (auto list = ui_troop_window.unit_list->get_component_t<cList>(); list)
				{
					list->set_count(player1.avaliable_unit_infos.size() + 1);

					if (auto image = ui_troop_window.unit_list->children[0]->get_component_t<cImage>(); image)
						image->set_image_name(L"assets\\icons\\red_cross.png");
					for (auto i = 0; i < player1.avaliable_unit_infos.size(); i++)
					{
						if (auto image = ui_troop_window.unit_list->children[i + 1]->get_component_t<cImage>(); image)
							image->set_image_name(player1.avaliable_unit_infos[i]->icon_name);
					}
				}
			}
		}
	}
};

// Reflect ctor dtor
struct EXPORT Action_formation_slot_click : Action
{
	// Reflect
	uint index;

	void exec() override
	{
		if (selected_unit_index == 0)
			player1.formation[index] = nullptr;
		else
			player1.formation[index] = player1.avaliable_unit_infos[selected_unit_index - 1];

		if (ui_troop_window.formation_area)
		{
			if (auto image = ui_troop_window.formation_area->children[index]->get_component_t<cImage>(); image)
			{
				if (!player1.formation[index])
					image->set_image_name(L"");
				else
					image->set_image_name(player1.formation[index]->icon_name);
			}
		}
	}
};

// Reflect ctor dtor
struct EXPORT Action_unit_slot_select : Action
{
	// Reflect
	uint index;

	void exec() override
	{
		selected_unit_index = index;

		if (ui_troop_window.unit_list)
		{
			for (auto& c : ui_troop_window.unit_list->children)
			{
				if (auto element = c->element(); element)
				{
					auto col = element->frame_col;
					col.a = 0;
					element->set_frame_col(col);
				}
			}

			if (auto element = ui_troop_window.unit_list->children[index]->element(); element)
			{
				auto col = element->frame_col;
				col.a = 255;
				element->set_frame_col(col);
			}
		}
	}
};

// Reflect ctor dtor
struct EXPORT Action_open_route_window : Action
{
	void exec() override
	{
		if (ui_route_window)
			ui_route_window->set_enable(!ui_route_window->enable);
		if (ui_building_window.window)
			ui_building_window.window->set_enable(false);
		if (ui_troop_window.window)
			ui_troop_window.window->set_enable(false);
	}
};

// Reflect ctor dtor
struct EXPORT Action_start_battle : Action
{
	void exec() override
	{
		if (game_state == GameStatePreparation)
			start_battle();
	}
};

graphics::CanvasPtr canvas = nullptr;
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
		ui_building_window.init(ui);
		ui_troop_window.init(ui);
		ui_route_window = ui->find_child("route_window");

		if (auto bottom_bar = ui->find_child("bottom_bar"); bottom_bar)
		{
			if (auto e = bottom_bar->find_child("hp_bar"); e)
			{
				ui_hp_bar = e->element();
				ui_hp_text = e->find_child("hp_text")->get_component_t<cText>();
			}
			if (auto e = bottom_bar->find_child("mp_bar"); e)
			{
				ui_mp_bar = e->element();
				ui_mp_text = e->find_child("mp_text")->get_component_t<cText>();
			}
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
	// TODO: show selected character
	//if (main_player.character)
	//{
	//	for (auto i = 0; i < 4; i++)
	//	{
	//		if (auto e = ui_ability_slots[i]; e)
	//		{
	//			if (auto ability = main_player.character->get_ability(i); ability)
	//			{
	//				if (auto img = e->get_component_t<cImage>(); img)
	//					img->set_image_name(ability->icon_name);
	//				if (auto e2 = e->find_child("mana"); e2)
	//				{
	//					if (auto txt = e2->get_component_t<cText>(); txt)
	//						txt->set_text(wstr(ability->mp));
	//				}
	//			}
	//			else
	//			{
	//				if (auto img = e->get_component_t<cImage>(); img)
	//					img->set_image_name(L"");
	//				if (auto e2 = e->find_child("mana"); e2)
	//				{
	//					if (auto txt = e2->get_component_t<cText>(); txt)
	//						txt->set_text(L"");
	//				}
	//			}

	//			if (ui_hp_bar)
	//				ui_hp_bar->set_scl(vec2((float)main_player.character->hp / (float)main_player.character->hp_max, 1.f));
	//			if (ui_mp_bar)
	//				ui_mp_bar->set_scl(vec2((float)main_player.character->mp / (float)main_player.character->mp_max, 1.f));
	//			if (ui_hp_text)
	//				ui_hp_text->set_text(wstr(main_player.character->hp) + L"/" + wstr(main_player.character->hp_max));
	//			if (ui_mp_text)
	//				ui_mp_text->set_text(wstr(main_player.character->mp) + L"/" + wstr(main_player.character->mp_max));
	//		}
	//	}
	//}

	std::wstring tip_str = L"";
	vec3 tip_pos;

	// TODO: show selected character
	//if (main_player.character)
	//{
	//	auto r = main_player.nav_agent->radius + 0.05f;
	//	auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
	//	std::vector<vec3> pts(circle_draw.pts.size() * 2);
	//	auto center = main_player.node->pos;
	//	center.y += 0.1f;
	//	for (auto i = 0; i < circle_draw.pts.size(); i++)
	//	{
	//		pts[i * 2 + 0] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
	//		pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
	//	}
	//	sRenderer::instance()->draw_primitives("LineList"_h, pts.data(), pts.size(), cvec4(85, 131, 79, 255), true);
	//}

	if (main_camera.camera)
	{
		auto camera_x = main_camera.node->x_axis();
		for (auto& character : find_characters_within_camera((FactionFlags)0xffffffff))
		{
			auto radius = character->get_radius();
			auto height = character->get_height();
			auto pos = character->node->pos;
			auto p0 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.1f, 0.f) - camera_x * radius);
			if (p0.x < 0.f)
				continue;
			auto p1 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.1f, 0.f) + camera_x * radius);
			if (p1.x < 0.f)
				continue;
			auto p2 = main_camera.camera->world_to_screen(pos + vec3(0.f, height, 0.f) + camera_x * radius);
			if (p2.x < 0.f)
				continue;
			auto w = p1.x - p0.x; auto h = p2.y - p0.y;
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
				sRenderer::instance()->draw_outlines(ds, hovering_character->faction == player1.faction ? 
					cvec4(64, 128, 64, 255) : cvec4(128, 64, 64, 255), 4, "BOX"_h);

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

	//if (select_distance > 0.f)
	//{
	//	if (select_range > 0.f)
	//	{
	//		auto r = select_range;
	//		auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
	//		auto center = hovering_pos;
	//		std::vector<vec3> pts(circle_draw.pts.size() * 3);
	//		for (auto i = 0; i < circle_draw.pts.size(); i++)
	//		{
	//			pts[i * 2 + 0] = center;
	//			pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
	//			pts[i * 2 + 2] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
	//		}

	//		sRenderer::instance()->draw_primitives("TriangleList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 100), true);
	//	}
	//	else
	//	{
	//		auto r = select_distance;
	//		auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
	//		auto center = main_player.node->pos;
	//		std::vector<vec3> pts(circle_draw.pts.size() * 3);
	//		for (auto i = 0; i < circle_draw.pts.size(); i++)
	//		{
	//			pts[i * 3 + 0] = center;
	//			pts[i * 3 + 1] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
	//			pts[i * 3 + 2] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
	//		}

	//		sRenderer::instance()->draw_primitives("TriangleList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 100), true);
	//	}
	//}

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
