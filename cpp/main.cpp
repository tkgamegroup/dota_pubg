#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/window.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

#include "main.h"
#include "character.h"
#include "spwaner.h"

EntityPtr root = nullptr;

static EntityPtr e_arrow = nullptr;

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
cCharacterPtr selecting_target = nullptr;

std::vector<vec3> site_positions;

cCharacterPtr hovering_character = nullptr;

cMain::~cMain()
{
	node->drawers.remove("main"_h);
}

void cMain::on_active()
{
	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		auto& tars = sRenderer::instance()->iv_tars;
		if (!tars.empty())
		{
			auto tar_size = vec2(tars.front()->image->size);
			ImGui::SetNextWindowPos(ImVec2(tar_size.x * 0.5f, tar_size.y), ImGuiCond_Always, ImVec2(0.5f, 1.f));
			ImGui::SetNextWindowSize(ImVec2(600.f, 160.f), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.5f);
			ImGui::Begin("##stats", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMouseInputs);
			if (selecting_target)
			{
				ImGui::TextUnformatted(selecting_target->entity->name.c_str());

				ImGui::BeginChild("##c1", ImVec2(100.f, -1.f));
				if (ImGui::BeginTable("##t1", 2));
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_sword = graphics::Image::get("assets\\icons\\sword.png");
					ImGui::Image(img_sword, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->atk);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_shield = graphics::Image::get("assets\\icons\\shield.png");
					ImGui::Image(img_shield, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->armor);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_run = graphics::Image::get("assets\\icons\\run.png");
					ImGui::Image(img_run, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->mov_sp);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_str = graphics::Image::get("assets\\icons\\strength.png");
					ImGui::Image(img_str, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->STR);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_agi = graphics::Image::get("assets\\icons\\agility.png");
					ImGui::Image(img_agi, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->AGI);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					static auto img_int = graphics::Image::get("assets\\icons\\intelligence.png");
					ImGui::Image(img_int, ImVec2(16, 16));
					ImGui::TableNextColumn();
					ImGui::Text("%5d", selecting_target->INT);

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
					dl->AddRectFilled(p0, p0 + vec2((float)selecting_target->hp / (float)selecting_target->hp_max * bar_width, bar_height), ImColor(0.3f, 0.7f, 0.2f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", selecting_target->hp, selecting_target->hp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				{
					ImGui::Dummy(ImVec2(bar_width, bar_height));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p0 + vec2((float)selecting_target->mp / (float)selecting_target->mp_max * bar_width, bar_height), ImColor(0.2f, 0.3f, 0.7f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", selecting_target->mp, selecting_target->mp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				{
					ImGui::Dummy(ImVec2(bar_width, bar_height));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p0 + vec2((float)selecting_target->exp / (float)selecting_target->exp_max * bar_width, bar_height), ImColor(0.7f, 0.7f, 0.2f));
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
					auto str = std::format("{}/{}", selecting_target->exp, selecting_target->exp_max);
					auto text_width = ImGui::CalcTextSize(str.c_str()).x;
					dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
				}
				if (selecting_target == main_player.character)
				{
					auto icon_size = 48.f;
					ImGui::BeginGroup();
					{
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "Q");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "W");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(2.f, 0.f), ImColor(1.f, 1.f, 1.f), "E");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "R");
					}
					ImGui::EndGroup();
					ImGui::SameLine();
					icon_size = 32.f;
					ImGui::BeginGroup();
					{
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "1");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "2");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "3");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "4");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "5");
					}
					{
						ImGui::SameLine();
						ImGui::Dummy(ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
						dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
						dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), "6");
					}
					ImGui::EndGroup();
				}
				ImGui::EndGroup();
			}
			ImGui::End();
		}
	}, (uint)this);
}

void cMain::on_inactive()
{
	graphics::gui_callbacks.remove((uint)this);
}

void cMain::start()
{
	srand(time(0));
	printf("Hello World\n");
	add_event([this]() {
		sScene::instance()->generate_nav_mesh();
		return false;
	});

	root = entity;

	e_arrow = Entity::create();
	e_arrow->load(L"assets/arrow.prefab");
	entity->add_child(e_arrow);

	main_camera.init(entity->find_child("Camera"));

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

				auto terrain_pos = terrain->node->pos;
				auto demon_pos = terrain_pos +
					site_positions[site_centrality.front().second] * terrain->extent;
				auto player1_pos = terrain_pos +
					site_positions[site_centrality.back().second] * terrain->extent;
				{
					auto e = Entity::create(); 
					e->load(L"assets/spawner.prefab");
					e->get_component_i<cNode>(0)->set_pos(demon_pos);
					auto spawner = e->get_component_t<cSpwaner>();
					spawner->spwan_interval = 1.f;
					spawner->spwan_count = 4;
					spawner->set_prefab_path(L"assets/monster.prefab");
					spawner->callbacks.add([spawner, player1_pos](EntityPtr e) {
						auto character = e->get_component_t<cCharacter>();
						character->faction = 2;
						character->nav_agent->separation_group = 2;
						add_event([character, player1_pos]() {
							character->cmd_move_attack(player1_pos);
							return false;
						});
						spawner->spwan_interval = 10000.f;
					});
					root->add_child(e);
				}
				{
					auto e = Entity::create();
					e->load(L"assets/main_player.prefab");
					e->get_component_i<cNode>(0)->set_pos(player1_pos + vec3(0.f, 0.f, -8.f));
					root->add_child(e);
					main_player.init(e);
					selecting_target = main_player.character;
				}
				{
					auto e = Entity::create();
					e->load(L"assets/spawner.prefab");
					e->get_component_i<cNode>(0)->set_pos(player1_pos);
					auto spawner = e->get_component_t<cSpwaner>();
					spawner->spwan_interval = 1.f;
					spawner->spwan_count = 4;
					spawner->set_prefab_path(L"assets/monster.prefab");
					spawner->callbacks.add([spawner, demon_pos](EntityPtr e) {
						auto character = e->get_component_t<cCharacter>();
						character->faction = 1;
						character->nav_agent->separation_group = 1;
						add_event([character, demon_pos]() {
							character->cmd_move_attack(demon_pos);
							return false;
						});
						spawner->spwan_interval = 10000.f;
					});
					root->add_child(e);
				}
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
		}
	}, "main"_h);
}

void cMain::update()
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
							draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, mesh->material_res_id);
					}
				}
			}
			break;
		case "terrain"_h:
			if (auto terrain = n->entity->get_component_t<cTerrain>(); terrain)
				draw_data.terrains.emplace_back(terrain->instance_id, product(terrain->blocks), terrain->material_res_id);
			break;
		}
		if (sInput::instance()->kpressed(Keyboard_F12))
			draw_data.graphics_debug = true;
	});
	hovering_character = nullptr;
	if (hovering_node)
	{
		if (auto character = hovering_node->entity->get_component_t<cCharacter>(); character)
			hovering_character = character;
	}
	if (input->mpressed(Mouse_Right))
	{
		if (hovering_character)
		{
			if (hovering_character->faction != main_player.character->faction)
				main_player.character->cmd_attack_target(hovering_character);
		}
		else if (hovering_node)
		{
			if (auto terrain = hovering_node->entity->get_component_t<cTerrain>(); terrain)
			{
				e_arrow->get_component_i<cNode>(0)->set_pos(hovering_pos);
				main_player.character->cmd_move_to(hovering_pos);
			}
		}
	}
	if (input->kpressed(Keyboard_A))
	{
		if (main_player.node)
		{
			//auto enemies = main_player.character->find_enemies();
			//if (!enemies.empty())
			//	main_player.character->enter_battle_state(enemies.front());
		}
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
