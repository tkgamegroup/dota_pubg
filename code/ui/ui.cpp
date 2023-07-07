#include <flame/graphics/canvas.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/element.h>
#include <flame/universe/components/receiver.h>
#include <flame/universe/components/layout.h>
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

bool selected_target_changed = true;

EntityPtr ui = nullptr;

graphics::CanvasPtr canvas = nullptr;
EntityPtr ui_resource_bar = nullptr;

struct BottomPanel
{
	enum PanelType
	{
		PanelNone,
		PanelCharacter,
		PanelTownMain,
		PanelTownBuilding
	};
	PanelType type = PanelNone;

	EntityPtr e = nullptr;
	cImagePtr avatar_image = nullptr;
	EntityPtr sub_panel1 = nullptr;
	cTextPtr name_text = nullptr;
	EntityPtr hp_bar = nullptr;
	EntityPtr mp_bar = nullptr;
	EntityPtr sub_panel2 = nullptr;
	EntityPtr sub_panel3 = nullptr;
	cListPtr training_list = nullptr;
	cListPtr action_list = nullptr;

	Player* player = nullptr;
	int building_index = -1;

	void init(EntityPtr _e)
	{
		e = _e;
		if (auto e = _e->find_child("avatar_image"); e)
			avatar_image = e->get_component_t<cImage>();
		sub_panel1 = _e->find_child("sub_panel1");
		if (sub_panel1)
		{
			if (auto e = sub_panel1->find_child("name"); e)
				name_text = e->get_component_t<cText>();
			hp_bar = sub_panel1->find_child("hp_bar");
			mp_bar = sub_panel1->find_child("mp_bar");
		}
		sub_panel2 = _e->find_child("sub_panel2");
		sub_panel3 = _e->find_child("sub_panel3");
		if (sub_panel3)
		{
			if (auto e = sub_panel3->find_child("training_list"); e)
				training_list = e->get_component_t<cList>();
		}
		if (auto e = _e->find_child("action_list"); e)
			action_list = e->get_component_t<cList>();
	}

	void reset()
	{
		type = PanelNone;
		if (avatar_image)
			avatar_image->set_image_name(L"");
		if (name_text)
		{
			name_text->set_col(cvec4(255));
			name_text->set_text(L"");
		}
		if (hp_bar)
			hp_bar->set_enable(false);
		if (mp_bar)
			mp_bar->set_enable(false);
		if (sub_panel3)
		{
			for (auto& c : sub_panel3->children)
			{
				if (auto list = c->get_parent_component_t<cList>(); list)
					list->set_count(0);
				c->set_enable(false);
			}
		}
		if (action_list)
		{
			for (auto& c : action_list->entity->children)
			{
				if (auto image = c->get_component_t<cImage>(); image)
					image->set_image_name(L"");
				if (auto receiver = c->get_component_t<cReceiver>(); receiver)
					receiver->event_listeners.clear();
			}
		}

		sInput::instance()->hovering_receiver = nullptr;
	}

	void enter_character_panel(cCharacterPtr character)
	{
		reset();
		type = PanelCharacter;
	}

	void enter_town_panel(Player* _player)
	{
		reset();
		type = PanelTownMain;
		player = _player;
		building_index = -1;

		if (name_text)
		{
			cvec4 col = cvec4(255);
			if (player == &player1)
				col = cvec4(100, 255, 100, 255);
			else if (player == &player2)
				col = cvec4(255, 100, 100, 255);
			name_text->set_col(col);
			name_text->set_text(s2w(selected_target->entity->name));
		}
		if (hp_bar)
			hp_bar->set_enable(true);

		if (action_list)
		{
			auto i = 0;
			for (auto& c : action_list->entity->children)
			{
				if (player == &player1 && i < player->buildings.size())
				{
					auto& building = player->buildings[i];
					if (auto image = c->get_component_t<cImage>(); image)
						image->set_image_name(building.info->icon_name);
					if (auto receiver = c->get_component_t<cReceiver>(); receiver)
					{
						receiver->event_listeners.clear();
						auto element = c->element();
						receiver->event_listeners.add([this, element, i](uint type, const vec2&) {
							switch (type)
							{
							case "mouse_enter"_h:
								show_tooltip(element->global_pos0() + vec2(0.f, -8.f), s2w(player->buildings[i].info->description));
								break;
							case "click"_h:
								enter_town_building_panel(i);
								break;
							}
						});
					}
				}
				i++;
			}
		}
	}

	void enter_town_building_panel(int index)
	{
		reset();
		type = PanelTownBuilding;
		building_index = index;

		if (name_text)
		{
			cvec4 col = cvec4(255);
			if (player == &player1)
				col = cvec4(100, 255, 100, 255);
			else if (player == &player2)
				col = cvec4(255, 100, 100, 255);
			name_text->set_col(col);
			name_text->set_text(s2w(selected_target->entity->name));
		}
		if (hp_bar)
			hp_bar->set_enable(true);

		if (action_list)
		{
			auto& building = player->buildings[index];
			if (!building.info->training_actions.empty())
			{
				auto i = 0;
				for (auto& c : action_list->entity->children)
				{
					if (i < building.info->training_actions.size())
					{
						auto& training = building.info->training_actions[i];
						auto unit_info = unit_infos.find(training.name);
						if (unit_info)
						{
							if (auto image = c->get_component_t<cImage>(); image)
								image->set_image_name(unit_info->icon_name);
							if (auto receiver = c->get_component_t<cReceiver>(); receiver)
							{
								receiver->event_listeners.clear();
								auto element = c->element();
								receiver->event_listeners.add([this, element, i](uint type, const vec2& v) {
									switch (type)
									{
									case "mouse_enter"_h:
									{
										auto& building = player->buildings[building_index];
										auto& training = building.info->training_actions[i];
										auto unit_info = unit_infos.find(training.name);
										if (unit_info)
										{
											std::wstring text;
											text = s2w(unit_info->name);
											text += L"\nLeft - Train One\n"
												L"Right - Train Infinite";
											show_tooltip(element->global_pos0() + vec2(0.f, -8.f), text,
												training.cost_blood, training.cost_bones, training.cost_soul_sand);
										}
									}
										break;
									case "click"_h:
									{
										auto& building = player->buildings[building_index];
										building.add_training(&building.info->training_actions[i], 1);
									}
										break;
									case "mouse_up"_h:
										if (v.x == 1.f)
										{
											auto& building = player->buildings[building_index];
											building.add_training(&building.info->training_actions[i], -1);
										}
										break;
									}
								});
							}
						}
					}
					i++;
				}

				auto columns = action_list->entity->get_component_t<cLayout>()->columns;
				auto last_row = (int)floor(((float)action_list->entity->children.size() - 0.5f) / columns);
				auto c = action_list->entity->children[last_row * columns].get();
				if (auto image = c->get_component_t<cImage>(); image)
					image->set_image_name(Path::get(L"assets\\extra\\icons\\back.png"));
				if (auto receiver = c->get_component_t<cReceiver>(); receiver)
				{
					receiver->event_listeners.clear();
					receiver->event_listeners.add([this, c](uint type, const vec2&) {
						switch (type)
						{
						case "mouse_enter"_h:
							show_tooltip(c->element()->global_pos0() + vec2(0.f, -8.f), L"Back To Main Panel");
							break;
						case "click"_h:
							enter_town_panel(player);
							break;
						}
					});
				}
			}
		}
	}

	void update()
	{
		if (selected_target_changed)
		{
			if (!selected_target)
				reset();
			else
			{
				if (selected_target->entity->name.ends_with("_town"))
				{
					Player* player = nullptr;
					if (selected_target->entity->name.starts_with("player1"))
						player = &player1;
					else if (selected_target->entity->name.starts_with("player2"))
						player = &player2;
					enter_town_panel(player);
				}
			}
		}

		switch (type)
		{
		case PanelTownMain:
			if (hp_bar)
			{
				hp_bar->find_child("bar")->element()->set_scl(vec2((float)player->town_hp / (float)player->town_hp_max, 1.f));
				hp_bar->find_child("text")->get_component_t<cText>()->set_text(wstr(player->town_hp) + L"/" + wstr(player->town_hp_max));
			}
			break;
		case PanelTownBuilding:
		{
			if (hp_bar)
			{
				hp_bar->find_child("bar")->element()->set_scl(vec2((float)player->town_hp / (float)player->town_hp_max, 1.f));
				hp_bar->find_child("text")->get_component_t<cText>()->set_text(wstr(player->town_hp) + L"/" + wstr(player->town_hp_max));
			}

			auto& building = player->buildings[building_index];
			if (training_list)
			{
				training_list->entity->set_enable(true);

				static uint training_updated_frame = 0;
				auto e_list = training_list->entity;
				if (training_updated_frame <= building.trainings_changed_frame)
				{
					training_updated_frame = building.trainings_changed_frame;

					training_list->set_count(building.trainings.size());
					for (auto i = 0; i < building.trainings.size(); i++)
					{
						auto c = e_list->children[i].get();
						auto& training = building.trainings[i];
						auto unit_info = unit_infos.find(training.action->name);
						if (unit_info)
						{
							if (auto image = c->find_child("icon")->get_component_t<cImage>(); image)
								image->set_image_name(unit_info->icon_name);
							auto cancel_button = c->find_child("cancel");
							if (cancel_button)
							{
								if (auto receiver = cancel_button->get_component_t<cReceiver>(); receiver)
								{
									receiver->event_listeners.clear();
									receiver->event_listeners.add([this, cancel_button, i](uint type, const vec2&) {
										switch (type)
										{
										case "mouse_enter"_h:
											show_tooltip(cancel_button->element()->global_pos0() + vec2(0.f, -8.f), L"Cancel");
											break;
										case "click"_h:
										{
											auto& building = player->buildings[building_index];
											building.remove_training(building.trainings[i].action);
										}
											break;
										}
									});
								}
							}
						}
						else
						{
							if (auto image = c->find_child("icon")->get_component_t<cImage>(); image)
								image->set_image_name(L"");
							if (auto receiver = c->find_child("cancel")->get_component_t<cReceiver>(); receiver)
								receiver->event_listeners.clear();
						}
					}
				}
				for (auto i = 0; i < building.trainings.size(); i++)
				{
					auto c = e_list->children[i].get();
					auto& training = building.trainings[i];
					c->find_child_recursively("number")->get_component_t<cText>()->set_text(wstr(training.number));
					c->find_child_recursively("bar")->element()->set_scl(vec2(1.f - training.timer / training.duration, 1.f));
					c->find_child_recursively("text")->get_component_t<cText>()->set_text(std::format(L"{}:{}", int(training.timer / 60.f), int(training.timer)));
				}
			}
		}
			break;
		}
	}
};

BottomPanel bottom_panel;
EntityPtr ui_tooltip = nullptr;

static int circle_lod(float r)
{
	return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
}

void init_ui()
{
	select_callbacks.add([]() {
		selected_target_changed = true;
	});

	auto renderer = sRenderer::instance();
	canvas = renderer->canvas;

	if (ui = root->find_child("ui"); ui)
	{
		if (auto top_bar = ui->find_child("top_bar"); top_bar)
			ui_resource_bar = top_bar->find_child("resources_bar");
		if (auto e = ui->find_child("bottom_panel"); e)
			bottom_panel.init(e);
		if (auto e = ui->find_child("tooltip"); e)
			ui_tooltip = e;
	}
}

void deinit_ui()
{
}

void update_ui()
{
	std::wstring tooltip_str = L"";
	vec2 tooltip_pos;

	if (ui_resource_bar)
	{
		ui_resource_bar->find_child("blood_icon")->set_enable(true);
		if (auto e = ui_resource_bar->find_child("blood_text"); e)
		{
			e->set_enable(true);
			e->get_component_t<cText>()->set_text(wstr(player1.blood));
		}
		ui_resource_bar->find_child("bones_icon")->set_enable(true);
		if (auto e = ui_resource_bar->find_child("bones_text"); e)
		{
			e->set_enable(true);
			e->get_component_t<cText>()->set_text(wstr(player1.bones));
		}
		ui_resource_bar->find_child("soul_sand_icon")->set_enable(true);
		if (auto e = ui_resource_bar->find_child("soul_sand_text"); e)
		{
			e->set_enable(true);
			e->get_component_t<cText>()->set_text(wstr(player1.soul_sand));
		}
	}

	bottom_panel.update();
	
	auto hovering_receiver = sInput::instance()->hovering_receiver;
	if (!hovering_receiver)
	{
		if (tooltip_str.empty())
		{
			if (hovering_character)
			{
				tooltip_str = s2w(hovering_character->entity->name);
				tooltip_pos = main_camera.camera->world_to_screen(hovering_character->get_pos() + vec3(0.f, 0.2f, 0.f));
			}
		}
		if (tooltip_str.empty())
			close_tooltip();
		else
		{
			if (main_camera.camera)
				show_tooltip(tooltip_pos, tooltip_str);
			else
				close_tooltip();
		}
	}

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

	selected_target_changed = false;
}

void show_tooltip(const vec2& pos, const std::wstring& text)
{
	if (ui_tooltip)
	{
		ui_tooltip->set_enable(true);
		ui_tooltip->element()->set_pos(pos);
		ui_tooltip->find_child("text")->get_component_t<cText>()->set_text(text);
		ui_tooltip->find_child("resources")->set_enable(false);
	}
}

void show_tooltip(const vec2& pos, const std::wstring& text, uint blood, uint bones, uint soul_sand)
{
	if (ui_tooltip)
	{
		ui_tooltip->set_enable(true);
		ui_tooltip->element()->set_pos(pos);
		ui_tooltip->find_child("text")->get_component_t<cText>()->set_text(text);
		auto e_resources = ui_tooltip->find_child("resources");
		e_resources->set_enable(true);
		if (blood > 0)
		{
			e_resources->find_child("blood_icon")->set_enable(true);
			if (auto e = e_resources->find_child("blood_text"); e)
			{
				e->set_enable(true);
				e->get_component_t<cText>()->set_text(wstr(blood));
			}
		}
		else
		{
			e_resources->find_child("blood_icon")->set_enable(false);
			e_resources->find_child("blood_text")->set_enable(false);
		}
		if (bones > 0)
		{
			e_resources->find_child("bones_icon")->set_enable(true);
			if (auto e = e_resources->find_child("bones_text"); e)
			{
				e->set_enable(true);
				e->get_component_t<cText>()->set_text(wstr(bones));
			}
		}
		else
		{
			e_resources->find_child("bones_icon")->set_enable(false);
			e_resources->find_child("bones_text")->set_enable(false);
		}
		if (soul_sand > 0)
		{
			e_resources->find_child("soul_sand_icon")->set_enable(true);
			if (auto e = e_resources->find_child("soul_sand_text"); e)
			{
				e->set_enable(true);
				e->get_component_t<cText>()->set_text(wstr(soul_sand));
			}
		}
		else
		{
			e_resources->find_child("soul_sand_icon")->set_enable(false);
			e_resources->find_child("soul_sand_text")->set_enable(false);
		}

	}
}

void close_tooltip()
{
	if (ui_tooltip)
	{
		ui_tooltip->set_enable(false);
		ui_tooltip->find_child("resources")->set_enable(false);
	}
}
