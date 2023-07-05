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

bool selected_target_changed = false;

EntityPtr ui = nullptr;

graphics::CanvasPtr canvas = nullptr;
cTextPtr ui_name_text = nullptr;
cElementPtr ui_hp_bar = nullptr;
cElementPtr ui_mp_bar = nullptr;
cTextPtr ui_hp_text = nullptr;
cTextPtr ui_mp_text = nullptr;
cListPtr ui_action_list = nullptr;
cTextPtr ui_tooltip = nullptr;

int circle_lod(float r)
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
		if (auto bottom_bar = ui->find_child("bottom_panel"); bottom_bar)
		{
			if (auto e = bottom_bar->find_child_recursively("name"); e)
				ui_name_text = e->get_component_t<cText>();
			if (auto e = bottom_bar->find_child_recursively("hp_bar"); e)
			{
				ui_hp_bar = e->find_child("bar")->element();
				ui_hp_text = e->find_child("text")->get_component_t<cText>();
			}
			if (auto e = bottom_bar->find_child_recursively("mp_bar"); e)
			{
				ui_mp_bar = e->find_child("bar")->element();
				ui_mp_text = e->find_child("text")->get_component_t<cText>();
			}
			if (auto e = bottom_bar->find_child_recursively("action_list"); e)
				ui_action_list = e->get_component_t<cList>();
		}

		if (auto e = ui->find_child("tooltip"); e)
			ui_tooltip = e->get_component_t<cText>();
	}
}

void deinit_ui()
{
}

void update_ui()
{
	if (selected_target)
	{
		if (auto character = selected_target->entity->get_component_t<cCharacter>(); character)
		{

		}
		else if (selected_target->entity->name.ends_with("_town"))
		{
			if (ui_name_text)
				ui_name_text->set_text(s2w(selected_target->entity->name));

			Player* player = nullptr;
			if (selected_target->entity->name.starts_with("player1"))
				player = &player1;
			else if (selected_target->entity->name.starts_with("player2"))
				player = &player2;

			if (ui_hp_bar)
				ui_hp_bar->set_scl(vec2((float)player->town_hp / (float)player->town_hp_max, 1.f));
			if (ui_mp_bar)
				ui_mp_bar->set_scl(vec2(0.f, 1.f));
			if (ui_hp_text)
				ui_hp_text->set_text(wstr(player->town_hp) + L"/" + wstr(player->town_hp_max));
			if (ui_mp_text)
				ui_mp_text->set_text(L"0/0");

			if (ui_action_list)
			{

				struct TownPanel
				{
					enum Panel
					{
						PanelMain,
						PanelBuilding
					};

					Panel panel = PanelMain;
					int building_index = -1;

					void enter_main_panel(Player* player)
					{
						panel = PanelMain;
						building_index = -1;

						auto e_list = ui_action_list->entity;
						for (auto i = 0; i < ui_action_list->count; i++)
						{
							auto c = e_list->children[i].get();
							if (i < player->buildings.size())
							{
								auto& building = player->buildings[i];
								if (auto image = c->get_component_t<cImage>(); image)
									image->set_image_name(building.info->icon_name);
								if (auto receiver = c->get_component_t<cReceiver>(); receiver)
								{
									receiver->event_listeners.clear();
									receiver->event_listeners.add([this, player, c, i](uint type, const vec2&) {
										switch (type)
										{
										case "mouse_enter"_h:
											show_tooltip(c->element()->global_pos0() + vec2(0.f, -48.f), s2w(player->buildings[i].info->description));
											break;
										case "mouse_leave"_h:
											close_tooltip();
											break;
										case "click"_h:
											enter_building_panel(player, i);
											close_tooltip();
											break;
										}
									});
								}
							}
							else
							{
								if (auto image = c->get_component_t<cImage>(); image)
									image->set_image_name(L"");
								if (auto receiver = c->get_component_t<cReceiver>(); receiver)
									receiver->event_listeners.clear();
							}
						}
					}

					void enter_building_panel(Player* player, uint index)
					{
						panel = PanelBuilding;
						building_index = index;

						auto e_list = ui_action_list->entity;
						auto& building = player->buildings[index];
						if (!building.info->training_actions.empty())
						{
							for (auto i = 0; i < ui_action_list->count; i++)
							{
								auto c = e_list->children[i].get();
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
											receiver->event_listeners.add([this, player, c, i](uint type, const vec2& v) {
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
														show_tooltip(c->element()->global_pos0() + vec2(0.f, -48.f), text);
													}
												}
													break;
												case "mouse_leave"_h:
													close_tooltip();
													break;
												case "click"_h:
												{
													auto& building = player->buildings[building_index];
													auto& action = building.info->training_actions[i];
													auto it = std::find_if(building.trainings.begin(), building.trainings.end(), [&](const auto& i) {
														return i.action == &action;
													});
													if (it == building.trainings.end())
													{
														it = building.trainings.emplace(building.trainings.end(), Training());
														it->action = &action;
														it->timer = action.time;
														it->number = 1;
													}
													else
														it->number++;
												}
													break;
												case "mouse_up"_h:
													if (v.x == 1.f)
													{

													}
													break;
												}
											});
										}
									}
									else
									{
										if (auto image = c->get_component_t<cImage>(); image)
											image->set_image_name(L"");
										if (auto receiver = c->get_component_t<cReceiver>(); receiver)
											receiver->event_listeners.clear();
									}
								}
								else
								{
									if (auto image = c->get_component_t<cImage>(); image)
										image->set_image_name(L"");
									if (auto receiver = c->get_component_t<cReceiver>(); receiver)
										receiver->event_listeners.clear();
								}
							}
							
							auto columns = e_list->get_component_t<cLayout>()->columns;
							auto last_row = (int)floor(((float)e_list->children.size() - 0.5f) / columns);
							auto c = e_list->children[last_row * columns].get();
							if (auto image = c->get_component_t<cImage>(); image)
								image->set_image_name(Path::get(L"assets\\extra\\icons\\back.png"));
							if (auto receiver = c->get_component_t<cReceiver>(); receiver)
							{
								receiver->event_listeners.clear();
								receiver->event_listeners.add([this, player, c](uint type, const vec2&) {
									switch (type)
									{
									case "mouse_enter"_h:
										show_tooltip(c->element()->global_pos0() + vec2(0.f, -48.f), L"Back To Main Panel");
										break;
									case "mouse_leave"_h:
										close_tooltip();
										break;
									case "click"_h:
										enter_main_panel(player);
										close_tooltip();
										break;
									}
								});
							}
						}
						else
						{
							for (auto i = 0; i < ui_action_list->count; i++)
							{
								auto c = e_list->children[i].get();
								if (auto image = c->get_component_t<cImage>(); image)
									image->set_image_name(L"");
								if (auto receiver = c->get_component_t<cReceiver>(); receiver)
									receiver->event_listeners.clear();
							}
						}
					}
				};
				static TownPanel town_panel;

				if (selected_target_changed)
					town_panel.enter_main_panel(player);
			}
		}
	}
	
	if (selected_target_changed)
	{
		std::wstring tooltip_str = L"";
		vec3 tooltip_pos;

		if (hovering_character)
		{
			tooltip_str = s2w(hovering_character->entity->name);
			tooltip_pos = hovering_character->get_pos() + vec3(0.f, 0.2f, 0.f);
		}

		if (tooltip_str.empty())
			close_tooltip();
		else
		{
			if (main_camera.camera)
				show_tooltip(main_camera.camera->world_to_screen(tooltip_pos), tooltip_str);
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
		ui_tooltip->entity->set_enable(true);
		ui_tooltip->set_text(text);
		ui_tooltip->element->set_pos(pos);
	}
}

void close_tooltip()
{
	if (ui_tooltip)
		ui_tooltip->entity->set_enable(false);
}
