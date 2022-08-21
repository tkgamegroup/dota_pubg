#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/foundation/window.h>
#include <flame/graphics/window.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

#include "main.h"
#include "item.h"
#include "character.h"
#include "spwaner.h"
#include "chest.h"

EntityPtr root = nullptr;

void MainCamera::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->get_component_i<cNode>(0);
		camera = e->get_component_t<cCamera>();
	}
}

void MainTerrain::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->get_component_i<cNode>(0);
		terrain = e->get_component_t<cTerrain>();
		extent = terrain->extent;
	}
}

vec3 MainTerrain::get_coord(const vec2& uv)
{
	return node->pos + vec3(uv.x, terrain->height_map->linear_sample(uv).x, uv.y) * extent;
}

vec3 MainTerrain::get_coord(const vec3& pos)
{
	return get_coord(vec2((pos.x - node->pos.x) / extent.x, (pos.z - node->pos.z) / extent.z));
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
MainTerrain main_terrain;
MainPlayer main_player;
cCharacterPtr selecting_character = nullptr;
cCharacterPtr hovering_character = nullptr;
cChestPtr hovering_chest = nullptr;

enum SelectMode
{
	SelectNull,
	SelectAttack
};
SelectMode select_mode = SelectNull;

cMain::~cMain()
{
	node->drawers.remove("main"_h);

	graphics::gui_callbacks.remove((uint)this);
	graphics::gui_cursor_callbacks.remove((uint)this);
}

void cMain::start()
{
	printf("main started\n");

	srand(time(0));

	add_event([this]() {
		sScene::instance()->generate_nav_mesh();

		return false;
	});

	root = entity;

	main_camera.init(entity->find_child("Camera"));
	main_terrain.init(entity->find_child("terrain"));
	{
		auto e = Entity::create();
		e->load(L"assets/main_player.prefab");
		root->add_child(e);
		main_player.init(e);
		main_player.character = main_player.character;
	}

	if (main_terrain.terrain)
	{
		std::vector<vec3> site_positions;
		if (auto height_map_info_fn = main_terrain.terrain->height_map->filename; !height_map_info_fn.empty())
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

		if (!site_positions.empty())
		{
			std::vector<std::pair<float, int>> site_centrality(site_positions.size());
			for (auto i = 0; i < site_positions.size(); i++)
			{
				auto x = abs(site_positions[i].x * 2.f - 1.f);
				auto z = abs(site_positions[i].z * 2.f - 1.f);
				site_centrality[i] = std::make_pair(x * z, i);
			}
			std::sort(site_centrality.begin(), site_centrality.end(), [](const auto& a, const auto& b) {
				return a.first < b.first;
			});

			auto demon_pos = site_positions[site_centrality.front().second].xz();
			auto player1_pos = site_positions[site_centrality.back().second].xz();
			auto demon_coord = main_terrain.get_coord(demon_pos);
			auto player1_coord = main_terrain.get_coord(player1_pos);

			{
				auto e = Entity::create();
				e->load(L"assets/spawner.prefab");
				e->get_component_i<cNode>(0)->set_pos(demon_coord);
				auto spawner = e->get_component_t<cSpwaner>();
				spawner->spwan_interval = 1.f;
				spawner->spwan_count = 4;
				spawner->set_prefab_path(L"assets/monster.prefab");
				spawner->callbacks.add([spawner, player1_coord](EntityPtr e) {
					auto character = e->get_component_t<cCharacter>();
					character->faction = 2;
					character->nav_agent->separation_group = 2;
					add_event([character, player1_coord]() {
						new CommandAttackLocation(character, player1_coord);
						return false;
					});
					spawner->spwan_interval = 10000.f;
				});
				root->add_child(e);
			}

			main_player.node->set_pos(main_terrain.get_coord(player1_coord + vec3(0.f, 0.f, -8.f)));
			add_chest(player1_coord + vec3(0.f, 0.f, -9.f))->item_id = 0;;

			{
				auto e = Entity::create();
				e->load(L"assets/spawner.prefab");
				e->get_component_i<cNode>(0)->set_pos(player1_coord);
				auto spawner = e->get_component_t<cSpwaner>();
				spawner->spwan_interval = 1.f;
				spawner->spwan_count = 4;
				spawner->set_prefab_path(L"assets/monster.prefab");
				spawner->callbacks.add([spawner, demon_coord](EntityPtr e) {
					auto character = e->get_component_t<cCharacter>();
					character->faction = 1;
					character->nav_agent->separation_group = 1;
					add_event([character, demon_coord]() {
						new CommandAttackLocation(character, demon_coord);
						return false;
					});
					spawner->spwan_interval = 10000.f;
				});
				root->add_child(e);
			}
		}
	}

	node->drawers.add([](DrawData& draw_data) {
		if (draw_data.pass == "outline"_h)
		{
			if (hovering_character)
			{
				if (auto armature = hovering_character->armature; armature && armature->model)
				{
					for (auto& c : armature->entity->children)
					{
						if (auto mesh = c->get_component_t<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
							draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, -1, 
								hovering_character->faction == main_player.character->faction ? cvec4(64, 128, 64, 255) : cvec4(128, 64, 64, 255));
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
		}
	}, "main"_h);

	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		auto tar_sz = sRenderer::instance()->target_size();
		if (tar_sz.x > 0.f && tar_sz.y > 0.f)
		{
			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_sz.x * 0.5f, tar_sz.y), ImGuiCond_Always, ImVec2(0.5f, 1.f));
			ImGui::SetNextWindowSize(ImVec2(600.f, 160.f), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.5f);
			ImGui::Begin("##stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
			if (main_player.character)
			{
				ImGui::TextUnformatted(main_player.character->entity->name.c_str());

				ImGui::BeginChild("##c1", ImVec2(100.f, -1.f));
				if (ImGui::BeginTable("##t1", 2));
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_sword = graphics::Image::get("assets\\icons\\sword.png");
					ImGui::Image(img_sword, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->atk);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_shield = graphics::Image::get("assets\\icons\\shield.png");
					ImGui::Image(img_shield, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->armor);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_run = graphics::Image::get("assets\\icons\\run.png");
					ImGui::Image(img_run, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->mov_sp);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_str = graphics::Image::get("assets\\icons\\strength.png");
					ImGui::Image(img_str, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->STR);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_agi = graphics::Image::get("assets\\icons\\agility.png");
					ImGui::Image(img_agi, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->AGI);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_int = graphics::Image::get("assets\\icons\\intelligence.png");
					ImGui::Image(img_int, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", main_player.character->INT);

					ImGui::EndTable();
				}
				ImGui::EndChild();

				ImGui::SameLine();

				ImGui::BeginGroup();
				auto dl = ImGui::GetWindowDrawList();
				const auto bar_width = ImGui::GetContentRegionAvailWidth();
				const auto bar_height = 16.f;
				{
					ImGui::Dummy(ImVec2(bar_width, bar_height));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->hp / (float)main_player.character->hp_max * bar_width, bar_height), ImColor(0.3f, 0.7f, 0.2f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", main_player.character->hp, main_player.character->hp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				{
					ImGui::Dummy(ImVec2(bar_width, bar_height));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->mp / (float)main_player.character->mp_max * bar_width, bar_height), ImColor(0.2f, 0.3f, 0.7f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", main_player.character->mp, main_player.character->mp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				{
					ImGui::Dummy(ImVec2(bar_width, bar_height));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->exp / (float)main_player.character->exp_max * bar_width, bar_height), ImColor(0.7f, 0.7f, 0.2f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", main_player.character->exp, main_player.character->exp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				if (main_player.character == main_player.character)
				{
					auto icon_size = 48.f;
					ImGui::BeginGroup();
					for (auto i = 0; i < 4; i++)
					{
						if (i > 0) ImGui::SameLine();
						static const char* names[] = {
							"ability_1", "ability_2", "ability_3", "ability_4", "ability_5", "ability_6"
						};
						ImGui::InvisibleButton(names[i], ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						static const char* hot_keys[] = {
							"Q", "W", "E", "R"
						};
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), hot_keys[i]);
					}
					ImGui::EndGroup();
					ImGui::SameLine();
					icon_size = 32.f;
					ImGui::BeginGroup();
					for (auto i = 0; i < 6; i++)
					{
						if (i > 0) ImGui::SameLine();
						static const char* names[] = {
							"item_1", "item_2", "item_3", "item_4", "item_5", "item_6"
						};
						ImGui::InvisibleButton(names[i], ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						auto id = main_player.character->inventory[i];
						if (id != -1)
						{
							auto& item = Item::get(id);
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::TextUnformatted(item.name.c_str());
								ImGui::EndTooltip();
							}
							if (ImGui::BeginDragDropSource())
							{
								ImGui::SetDragDropPayload("item", &i, sizeof(int));
								ImGui::EndDragDropSource();
							}
							if (ImGui::BeginPopupContextItem())
							{
								if (ImGui::Selectable("Drop"))
								{
									main_player.character->inventory[i] = -1;
									add_chest(main_player.character->node->g_pos + vec3(linearRand(-0.2f, 0.2f), 0.f, linearRand(-0.2f, 0.2f)))->item_id = id;
								}
								ImGui::EndPopup();
							}
							dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());
						}
						if (ImGui::BeginDragDropTarget())
						{
							if (auto payload = ImGui::AcceptDragDropPayload("item"); payload)
							{
								auto j = *(int*)payload->Data;
								if (i != j)
									std::swap(main_player.character->inventory[i], main_player.character->inventory[j]);
							}
							ImGui::EndDragDropTarget();
						}
						static const char* hot_keys[] = {
							"1", "2", "3", "4", "5", "6"
						};
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), hot_keys[i]);
					}
					ImGui::EndGroup();
				}
				ImGui::EndGroup();
			}
			ImGui::End();

			static graphics::ImagePtr icon_cursors = nullptr;
			if (!icon_cursors)
				icon_cursors = graphics::Image::get(L"assets\\icons\\rpg_cursor_set.png");
			const auto cursor_cx = 6U;
			const auto cursor_cy = 3U;
			int cursor_x = 0, cursor_y = 0;
			switch (select_mode)
			{
			case SelectAttack:
				cursor_x = 3;
				cursor_y = 0;
				break;
			}
			auto pos = sInput::instance()->mpos;
			auto dl = ImGui::GetForegroundDrawList();
			dl->AddImage(icon_cursors, pos + vec2(-32.f), pos + vec2(32.f),
				vec2((float)cursor_x / cursor_cx, (float)cursor_y / cursor_cy),
				vec2((float)(cursor_x + 1) / cursor_cx, (float)(cursor_y + 1) / cursor_cy));
		}
	}, (uint)this);
	graphics::gui_cursor_callbacks.add([this](CursorType cursor) {
		auto mpos = sInput::instance()->mpos;
		auto screen_sz = sRenderer::instance()->target_size();
		if (mpos.x < 0.f || mpos.x > screen_sz.x || mpos.y < 0.f || mpos.y > screen_sz.y)
			return cursor;
		return CursorNone;
	}, (uint)this);

	load_items();
}

void cMain::update()
{
	if (!graphics::gui_want_mouse())
	{
		auto input = sInput::instance();
		vec3 hovering_pos;
		auto hovering_node = sRenderer::instance()->pick_up(input->mpos, &hovering_pos, [](cNodePtr n, DrawData& draw_data) {
			switch (draw_data.category)
			{
			case "mesh"_h:
				if (auto character = n->entity->get_component_t<cCharacter>(); character)
				{
					if (character != main_player.character && character->armature)
					{
						for (auto& c : character->armature->entity->children)
						{
							if (auto mesh = c->get_component_t<cMesh>(); mesh)
							{
								if (mesh->instance_id != -1 && mesh->mesh_res_id != -1 && mesh->material_res_id != -1)
									draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, mesh->material_res_id);
							}
						}
					}
				}
				if (auto chest = n->entity->get_component_t<cChest>(); chest)
				{
					for (auto& c : chest->entity->get_all_children())
					{
						if (auto mesh = c->get_component_t<cMesh>(); mesh)
						{
							if (mesh->instance_id != -1 && mesh->mesh_res_id != -1 && mesh->material_res_id != -1)
								draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, mesh->material_res_id);
						}
					}
				}
				break;
			case "terrain"_h:
				if (auto terrain = n->entity->get_component_t<cTerrain>(); terrain)
				{
					if (terrain->instance_id != -1 && terrain->material_res_id != -1)
						draw_data.terrains.emplace_back(terrain->instance_id, product(terrain->blocks), terrain->material_res_id);
				}
				break;
			}
			if (sInput::instance()->kpressed(Keyboard_F12))
				draw_data.graphics_debug = true;
		});
		hovering_character = nullptr;
		hovering_chest = nullptr;
		if (hovering_node)
		{
			if (auto character = hovering_node->entity->get_component_t<cCharacter>(); character)
				hovering_character = character;
			if (auto chest = hovering_node->entity->get_component_t<cChest>(); chest)
				hovering_chest = chest;
		}
		auto add_location_icon = [](const vec3& pos, const vec3& color) {
			static graphics::ImagePtr icon_location = nullptr;
			if (!icon_location)
				icon_location = graphics::Image::get(L"assets\\icons\\location.png");
			static int icon_location_id = 0;
			auto ticks = 30;
			auto _id = icon_location_id;
			graphics::gui_callbacks.add([pos, color, ticks, _id]() mutable {
				auto p = main_camera.camera->world_to_screen(pos);
				if (p.x > 0.f && p.y > 0.f)
				{
					p.xy += sInput::instance()->offset;
					auto dl = ImGui::GetForegroundDrawList();
					auto sz = (vec2)icon_location->size;
					dl->AddImage(icon_location, p - vec2(sz.x * 0.5f, sz.y), p + vec2(sz.x * 0.5f, 0.f), vec2(0.f), vec2(1.f), ImColor(color.r, color.g, color.b, max(0.f, ticks / 30.f)));
				}
				if (ticks-- <= 0)
				{
					auto h = sh(("icon_location_" + str(_id)).c_str());
					add_event([h]() {
						graphics::gui_callbacks.remove(h);
						return false;
					});
				}
			}, sh(("icon_location_" + str(_id)).c_str()));
			icon_location_id++;
		};
		if (input->mpressed(Mouse_Left))
		{
			if (select_mode == SelectNull)
				;
			else
			{
				switch (select_mode)
				{
				case SelectAttack:
					if (hovering_character)
					{
						if (hovering_character->faction != main_player.character->faction)
						{
							new CommandAttackTarget(main_player.character, hovering_character);
							select_mode = SelectNull;
						}
					}
					else if (hovering_node)
					{
						if (auto terrain = hovering_node->entity->get_component_t<cTerrain>(); terrain)
						{
							new CommandMoveTo(main_player.character, hovering_pos);
							add_location_icon(hovering_pos, vec3(1.f, 0.f, 0.f));
							select_mode = SelectNull;
						}
					}
					break;
				}
			}
		}
		if (input->mpressed(Mouse_Right))
		{
			if (select_mode == SelectNull)
			{
				if (hovering_character)
				{
					if (hovering_character->faction != main_player.character->faction)
						new CommandAttackTarget(main_player.character, hovering_character);
				}
				else if (hovering_chest)
				{
					new CommandPickUp(main_player.character, hovering_chest);
				}
				else if (hovering_node)
				{
					if (auto terrain = hovering_node->entity->get_component_t<cTerrain>(); terrain)
					{
						new CommandMoveTo(main_player.character, hovering_pos);
						add_location_icon(hovering_pos, vec3(0.f, 1.f, 0.f));
					}
				}
			}
			else
				select_mode = SelectNull;
		}

		if (input->kpressed(Keyboard_Esc))
			select_mode = SelectNull;
		if (input->kpressed(Keyboard_A))
			select_mode = SelectAttack;
	}

	if (main_camera.node && main_player.node)
	{
		static vec3 velocity(0.f);
		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
		main_camera.node->set_pos(smooth_damp(main_camera.node->pos, main_player.node->g_pos + camera_length * main_camera.node->g_rot[2], velocity, 0.3f, 10000.f, delta_time));
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

cChestPtr add_chest(const vec3& pos)
{
	static EntityPtr e_chest = nullptr;
	if (!e_chest)
	{
		e_chest = Entity::create();
		e_chest->load(L"assets\\models\\chest.prefab");
	}
	auto e = e_chest->copy();
	e->get_component_i<cNode>(0)->set_pos(main_terrain.get_coord(pos));
	root->add_child(e);
	return e->get_component_t<cChest>();
}

void pick_up_chest(cCharacterPtr character, cChestPtr chest)
{
	auto ok = false;
	for (auto i = 0; i < countof(character->inventory); i++)
	{
		if (character->inventory[i] == -1)
		{
			character->inventory[i] = chest->item_id;
			character->stats_dirty = true;
			ok = true;
			break;
		}
	}
	if (ok)
	{
		auto e = chest->entity;
		add_event([e]() {
			e->remove_from_parent();
			return false;
		});
	}
}

EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cMain::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cSpwaner::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}
