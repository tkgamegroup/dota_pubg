#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/foundation/window.h>
#include <flame/graphics/window.h>
#include <flame/graphics/gui.h>
#include <flame/universe/entity.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/octree.h>
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
#include "ability.h"
#include "buff.h"
#include "character.h"
#include "spwaner.h"
#include "projectile.h"
#include "chest.h"

EntityPtr root = nullptr;

void MainCamera::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->node();
		camera = e->get_component_t<cCamera>();
	}
}

void MainTerrain::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->node();
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
		node = e->node();
		nav_agent = e->get_component_t<cNavAgent>();
		character = e->get_component_t<cCharacter>();
	}
}

MainCamera main_camera;
MainTerrain main_terrain;
MainPlayer main_player;

cCharacterPtr	hovering_character = nullptr;
cChestPtr		hovering_chest = nullptr;
cTerrainPtr		hovering_terrain = nullptr;

TargetType								select_mode = TargetNull;
std::function<void(cCharacterPtr)>		select_enemy_callback;
std::function<void(const vec3& pos)>	select_location_callback;
float									select_distance = 0.f;
float									select_range = 0.f;
void reset_select()
{
	select_mode = TargetNull;
	select_enemy_callback = nullptr;
	select_location_callback = nullptr;
	select_distance = 0.f;
	select_range = 0.f;
}

cCharacterPtr focus_character = nullptr;

std::string illegal_op_str;
float illegal_op_str_timer = 0.f;

void click_ability(AbilityInstance* ins)
{
	if (ins->cd_timer > 0.f)
	{
		illegal_op_str = "Cooldowning.";
		illegal_op_str_timer = 3.f;
		return;
	}
	auto& ability = Ability::get(ins->id);
	if (main_player.character->mp < ability.mana)
	{
		illegal_op_str = "Not Enough Mana.";
		illegal_op_str_timer = 3.f;
		return;
	}
	select_mode = ability.target_type;
	if (ability.target_type & TargetLocation)
	{
		select_location_callback = [ins](const vec3& location) {
			new CommandCastAbilityToLocation(main_player.character, ins, location);
		};
	}
	if (ability.target_type & TargetEnemy)
	{
		select_enemy_callback = [ins](cCharacterPtr character) {
			new CommandCastAbilityToTarget(main_player.character, ins, character);
		};
		select_distance = ability.distance;
	}
}

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
		e->load(L"assets\\characters\\dragon_knight\\main.prefab");
		root->add_child(e);
		main_player.init(e);
		main_player.character->set_faction(1);
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

			auto player1_pos = site_positions[site_centrality.back().second].xz();
			auto player1_coord = main_terrain.get_coord(player1_pos);

			main_player.node->set_pos(main_terrain.get_coord(player1_coord));
			add_chest(player1_coord + vec3(2.f, 0.f, 0.f), Item::find("Boots of Speed"));
			add_chest(player1_coord + vec3(0.f, 0.f, 2.f), Item::find("Magic Candy"));

			for (auto i = 1; i < site_centrality.size() - 1; i++)
			{
				auto coord = main_terrain.get_coord(site_positions[site_centrality[i].second].xz());

				static const wchar_t* prefabs[] = {
					L"assets\\characters\\life_stealer\\main.prefab",
					L"assets\\characters\\slark\\main.prefab"
				};

				auto e = Entity::create();
				e->load(prefabs[linearRand(0U, (uint)countof(prefabs) - 1)]);
				e->node()->set_pos(coord);
				auto character = e->get_component_t<cCharacter>();
				character->set_faction(2);
				new CommandAttackLocation(character, coord);
				root->add_child(e);
			}
			for (auto i = 0; i < 100; i++)
			{
				auto coord = main_terrain.get_coord(vec2(linearRand(0.f, 1.f), linearRand(0.f, 1.f)));

				auto e = Entity::create();
				e->load(L"assets\\characters\\spiderling\\main.prefab");
				e->node()->set_pos(coord);
				auto character = e->get_component_t<cCharacter>();
				character->set_faction(2);
				character->set_preset_name("Spiderling");
				new CommandAttackLocation(character, coord);
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
		else if (draw_data.pass == "primitive"_h)
		{
			if (select_distance > 0.f)
			{
				auto r = select_distance;
				auto circle_pts = graphics::get_circle_points(r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0))));
				auto n = (int)circle_pts.size();
				circle_pts.push_back(circle_pts[0]);
				std::vector<vec3> pts(n * 2);
				auto center = main_player.character->node->pos;
				for (auto i = 0; i < n; i++)
				{
					pts[i * 2 + 0] = center + vec3(r * circle_pts[i + 0], 0.f).xzy();
					pts[i * 2 + 1] = center + vec3(r * circle_pts[i + 1], 0.f).xzy();
				}
				draw_data.primitives.emplace_back("LineList"_h, pts.data(), (uint)pts.size(), cvec4(0, 255, 0, 255));
			}
		}
	}, "main"_h);

	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		auto tar_sz = sRenderer::instance()->target_size();
		if (tar_sz.x <= 0.f || tar_sz.y <= 0.f)
			return;
		ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_sz.x * 0.5f, tar_sz.y), ImGuiCond_Always, ImVec2(0.5f, 1.f));
		ImGui::SetNextWindowSize(ImVec2(600.f, 100.f), ImGuiCond_Always);
		ImGui::Begin("##main_panel", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
		if (main_player.character)
		{
			ImGui::TextUnformatted(main_player.character->entity->name.c_str());

			ImGui::BeginChild("##c1", ImVec2(100.f, -1.f));
			//if (ImGui::BeginTable("##t1", 2));
			//{
			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_sword = graphics::Image::get("assets\\icons\\sword.png");
			//	ImGui::Image(img_sword, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->atk);

			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_shield = graphics::Image::get("assets\\icons\\shield.png");
			//	ImGui::Image(img_shield, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->armor);

			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_run = graphics::Image::get("assets\\icons\\run.png");
			//	ImGui::Image(img_run, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->mov_sp);

			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_str = graphics::Image::get("assets\\icons\\strength.png");
			//	ImGui::Image(img_str, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->STR);

			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_agi = graphics::Image::get("assets\\icons\\agility.png");
			//	ImGui::Image(img_agi, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->AGI);

			//	ImGui::TableNextRow();
			//	ImGui::TableNextColumn();
			//	static auto img_int = graphics::Image::get("assets\\icons\\intelligence.png");
			//	ImGui::Image(img_int, ImVec2(16, 16));
			//	ImGui::TableNextColumn();
			//	ImGui::Text("%5d", main_player.character->INT);

			//	ImGui::EndTable();
			//}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginGroup();
			auto dl = ImGui::GetWindowDrawList();
			const auto bar_width = ImGui::GetContentRegionAvailWidth();
			const auto bar_height = 16.f;

			{
				auto icon_size = 48.f;
				ImGui::BeginGroup();
				for (auto i = 0; i < 4; i++)
				{
					if (i > 0) ImGui::SameLine();
					static const char* names[] = {
						"ability_1", "ability_2", "ability_3", "ability_4", "ability_5", "ability_6"
					};
					auto pressed = ImGui::InvisibleButton(names[i], ImVec2(icon_size, icon_size));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
					auto ins = main_player.character->abilities[i].get();
					if (ins)
					{
						auto& ability = Ability::get(ins->id);
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::TextUnformatted(ability.name.c_str());
							ImGui::EndTooltip();
						}
						dl->AddImage(ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());

						if (ins->cd_max > 0.f && ins->cd_timer > 0.f)
							dl->AddRectFilled(p0, vec2(p1.x, mix(p0.y, p1.y, ins->cd_timer / ins->cd_max)), ImColor(0.f, 0.f, 0.f, 0.5f));

						if (pressed)
							click_ability(ins);
					}
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
					auto pressed = ImGui::InvisibleButton(names[i], ImVec2(icon_size, icon_size));
					auto p0 = (vec2)ImGui::GetItemRectMin();
					auto p1 = (vec2)ImGui::GetItemRectMax();
					dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
					auto& ins = main_player.character->inventory[i];
					if (ins)
					{
						auto& item = Item::get(ins->id);
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
								add_chest(main_player.character->node->pos + vec3(linearRand(-0.2f, 0.2f), 0.f, linearRand(-0.2f, 0.2f)), ins->id, ins->num);
								main_player.character->inventory[i].reset(nullptr);
							}
							ImGui::EndPopup();
						}
						dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());

						if (pressed)
							main_player.character->use_item(ins.get());
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
					dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
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

		{
			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(8.f, 4.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
			ImGui::SetNextWindowSize(ImVec2(200.f, 100.f), ImGuiCond_Always);
			ImGui::Begin("##main_player", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

			auto dl = ImGui::GetWindowDrawList();
			const auto bar_width = ImGui::GetContentRegionAvailWidth();
			const auto bar_height = 16.f;
			{
				ImGui::Dummy(ImVec2(bar_width, bar_height));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->hp / (float)main_player.character->hp_max * bar_width, bar_height), ImColor(0.3f, 0.7f, 0.2f));
				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
				auto str = std::format("{}/{}", main_player.character->hp / 10, main_player.character->hp_max / 10);
				auto text_width = ImGui::CalcTextSize(str.c_str()).x;
				dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
			}
			{
				ImGui::Dummy(ImVec2(bar_width, bar_height));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->mp / (float)main_player.character->mp_max * bar_width, bar_height), ImColor(0.2f, 0.3f, 0.7f));
				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
				auto str = std::format("{}/{}", main_player.character->mp / 10, main_player.character->mp_max / 10);
				auto text_width = ImGui::CalcTextSize(str.c_str()).x;
				dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
			}
			{
				ImGui::Dummy(ImVec2(bar_width, bar_height));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)main_player.character->exp / (float)main_player.character->exp_max * bar_width, bar_height), ImColor(0.7f, 0.7f, 0.2f));
				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
				auto str = std::format("LV {}  {}/{}", main_player.character->lv, main_player.character->exp, main_player.character->exp_max);
				auto text_width = ImGui::CalcTextSize(str.c_str()).x;
				dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
			}

			ImGui::End();
		}

		if (focus_character)
		{
			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_sz.x - 8.f, 4.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
			ImGui::SetNextWindowSize(ImVec2(100.f, 100.f), ImGuiCond_Always);
			ImGui::Begin("##focus_character", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

			auto dl = ImGui::GetWindowDrawList();
			const auto bar_width = ImGui::GetContentRegionAvailWidth();
			const auto bar_height = 16.f;
			{
				ImGui::Dummy(ImVec2(bar_width, bar_height));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)focus_character->hp / (float)focus_character->hp_max * bar_width, bar_height), ImColor(0.3f, 0.7f, 0.2f));
				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
				auto str = std::format("{}/{}", focus_character->hp / 10, focus_character->hp_max / 10);
				auto text_width = ImGui::CalcTextSize(str.c_str()).x;
				dl->AddText(p0 + vec2((bar_width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
			}
			auto icon_size = 32.f;
			for (auto i = 0; i < focus_character->buffs.size(); i++)
			{
				if (i > 0) ImGui::SameLine();
				auto& ins = focus_character->buffs[i];
				auto& buff = Buff::get(ins->id);
				ImGui::Dummy(ImVec2(icon_size, icon_size));
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(buff.name.c_str());
					ImGui::EndTooltip();
				}
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddImage(buff.icon_image, p0, p1, buff.icon_uvs.xy(), buff.icon_uvs.zw());
			}

			ImGui::End();
		}

		if (illegal_op_str_timer > 0.f)
		{
			auto dl = ImGui::GetForegroundDrawList();
			auto text_size = (vec2)ImGui::CalcTextSize(illegal_op_str.c_str());
			auto p = vec2((tar_sz.x - text_size.x) * 0.5f, tar_sz.y - 160.f - text_size.y);
			p.xy += sInput::instance()->offset;
			auto alpha = 1.f;
			if (illegal_op_str_timer < 1.f)
				alpha *= mix(0.f, 1.f, illegal_op_str_timer);
			auto border = 0.f;
			if (illegal_op_str_timer > 2.9f)
				border = mix(8.f, 0.f, (illegal_op_str_timer - 2.9f) / 0.1f);
			else if (illegal_op_str_timer > 2.5f)
				border = mix(0.f, 8.f, (illegal_op_str_timer - 2.5f) / 0.4f);
			dl->AddRectFilled(p - vec2(2.f + border), p + text_size + vec2(2.f + border), ImColor(1.f, 0.f, 0.f, 0.5f * alpha));
			dl->AddText(p, ImColor(1.f, 1.f, 1.f, 1.f * alpha), illegal_op_str.c_str());
			illegal_op_str_timer -= delta_time;
		}

		{
			static graphics::ImagePtr icon_cursors = nullptr;
			if (!icon_cursors)
				icon_cursors = graphics::Image::get(L"assets\\icons\\rpg_cursor_set.png");
			const auto cursor_cx = 6U;
			const auto cursor_cy = 3U;
			int cursor_x = 0, cursor_y = 0;
			if (select_mode != TargetNull)
			{
				cursor_x = 3;
				cursor_y = 0;
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
		hovering_terrain = nullptr;
		if (hovering_node)
		{
			auto can_select = [](cCharacterPtr character) {
				if (!(select_mode & TargetEnemy) && main_player.character->faction != character->faction)
					return false;
				if (!(select_mode & TargetFriendly) && main_player.character->faction == character->faction)
					return false;
				return true;
			};
			if (auto character = hovering_node->entity->get_component_t<cCharacter>(); character)
			{
				if (select_mode == TargetNull || can_select(character))
					hovering_character = character;
			}
			if (auto chest = hovering_node->entity->get_component_t<cChest>(); chest)
				hovering_chest = chest;
			if (auto terrain = hovering_node->entity->get_component_t<cTerrain>(); terrain)
			{
				hovering_terrain = terrain;

				if (!(select_mode & TargetLocation))
				{
					if ((select_mode & TargetEnemy) || (select_mode & TargetFriendly))
					{
						std::vector<cNodePtr> objs;
						sScene::instance()->octree->get_colliding(hovering_pos, 5.f, objs, CharacterTag);
						if (!objs.empty())
						{
							auto min_dist = 10000.f;
							for (auto n : objs)
							{
								if (auto character = n->entity->get_component_t<cCharacter>(); character)
								{
									if (!can_select(character))
										continue;
									auto dist = distance(n->pos, hovering_pos);
									if (dist < min_dist)
									{
										hovering_character = character;
										min_dist = dist;
									}
								}
							}
						}
					}
				}
			}
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
			if (select_mode == TargetNull)
			{
				if (hovering_character)
					focus_character = hovering_character;
				else
					focus_character = nullptr;
			}
			else
			{
				if (select_mode & TargetEnemy)
				{
					if (hovering_character && main_player.character->faction != hovering_character->faction)
					{
						if (select_enemy_callback)
							select_enemy_callback(hovering_character);
						reset_select();
					}
				}
				if (select_mode & TargetLocation)
				{
					if (hovering_terrain)
					{
						if (select_location_callback)
							select_location_callback(hovering_pos);
						reset_select();
					}
				}
			}
		}
		if (input->mpressed(Mouse_Right))
		{
			if (select_mode == TargetNull)
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
				else if (hovering_terrain)
				{
					new CommandMoveTo(main_player.character, hovering_pos);
					add_location_icon(hovering_pos, vec3(0.f, 1.f, 0.f));
				}
			}
			else
				reset_select();
		}

		if (input->kpressed(Keyboard_Esc))
			reset_select();
		if (input->kpressed(Keyboard_A))
		{
			select_mode = TargetType(TargetEnemy | TargetLocation);
			select_enemy_callback = [](cCharacterPtr character) {
				new CommandAttackTarget(main_player.character, character);
			};
			select_location_callback = [](const vec3& pos) {
				new CommandAttackLocation(main_player.character, pos);
			};
		}
		if (input->kpressed(Keyboard_Q))
		{
			auto ins = main_player.character->abilities[0].get();
			if (ins)
				click_ability(ins);
		}
		if (input->kpressed(Keyboard_W))
		{
			auto ins = main_player.character->abilities[1].get();
			if (ins)
				click_ability(ins);
		}
		if (input->kpressed(Keyboard_E))
		{
			auto ins = main_player.character->abilities[2].get();
			if (ins)
				click_ability(ins);
		}
		if (input->kpressed(Keyboard_R))
		{
			auto ins = main_player.character->abilities[3].get();
			if (ins)
				click_ability(ins);
		}
	}

	if (main_camera.node && main_player.node)
	{
		static vec3 velocity(0.f);
		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
		main_camera.node->set_pos(smooth_damp(main_camera.node->pos, main_player.node->pos + camera_length * main_camera.node->g_rot[2], velocity, 0.3f, 10000.f, delta_time));
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

std::vector<cCharacterPtr> get_characters(const vec3& pos, float radius, uint faction)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos, radius, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
			ret.push_back(chr);
	}

	std::vector<std::pair<float, cCharacterPtr>> dist_list(ret.size());
	for (auto i = 0; i < ret.size(); i++)
	{
		auto c = ret[i];
		dist_list[i] = std::make_pair(distance(c->node->pos, pos), c);
	}
	std::sort(dist_list.begin(), dist_list.end(), [](const auto& a, const auto& b) {
		return a.first < b.first;
	});
	for (auto i = 0; i < ret.size(); i++)
		ret[i] = dist_list[i].second;
	return ret;
}

void add_projectile(EntityPtr prefab, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(cCharacterPtr t)>& cb)
{
	auto e = prefab->copy();
	e->get_component_t<cNode>()->set_pos(pos);
	auto projectile = e->get_component_t<cProjectile>();
	projectile->target = target;
	projectile->speed = speed;
	projectile->callback = cb;
	root->add_child(e);
}

void add_chest(const vec3& pos, uint item_id, uint item_num)
{
	static EntityPtr e_chest = nullptr;
	if (!e_chest)
	{
		e_chest = Entity::create();
		e_chest->load(L"assets\\models\\chest.prefab");
	}
	auto e = e_chest->copy();
	e->node()->set_pos(main_terrain.get_coord(pos));
	root->add_child(e);
	auto chest = e->get_component_t<cChest>();
	chest->item_id = item_id;
	chest->item_num = item_num;
}

void teleport(cCharacterPtr character, const vec3& location)
{
	character->node->set_pos(location);
	character->nav_agent->update_pos();
}

EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cMain::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cSpwaner::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}
