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
#include "views/view_equipment.h"
#include "views/view_ability.h"
#include "views/view_inventory.h"

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

std::string illegal_op_str;
float illegal_op_str_timer = 0.f;

std::string tooltip;

struct Shortcut
{
	KeyboardKey key = KeyboardKey_Count;

	virtual void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) {}
	virtual void click() {}
};

struct ItemShortcut : Shortcut
{
	ItemInstance* ins;

	ItemShortcut(ItemInstance* ins) :
		ins(ins)
	{
	}

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override
	{
		auto& item = Item::get(ins->id);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(item.name.c_str());
			if (item.show)
				item.show();
			ImGui::EndTooltip();
		}
		dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());
	}

	void click() override
	{
		main_player.character->use_item(ins);
	}
};

struct AbilityShortcut : Shortcut
{
	AbilityInstance* ins;

	AbilityShortcut(AbilityInstance* ins) :
		ins(ins)
	{
	}

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override
	{
		auto& ability = Ability::get(ins->id);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(ability.name.c_str());
			if (ability.show)
				ability.show();
			ImGui::EndTooltip();
		}
		dl->AddImage(ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());

		if (ins->cd_max > 0.f && ins->cd_timer > 0.f)
			dl->AddRectFilled(p0, vec2(p1.x, mix(p0.y, p1.y, ins->cd_timer / ins->cd_max)), ImColor(0.f, 0.f, 0.f, 0.5f));
	}

	void click() override
	{
		if (ins->cd_timer > 0.f)
		{
			illegal_op_str = "Cooldowning.";
			illegal_op_str_timer = 3.f;
			return;
		}
		auto& ability = Ability::get(ins->id);
		if (main_player.character->mp < ability.mp)
		{
			illegal_op_str = "Not Enough MP.";
			illegal_op_str_timer = 3.f;
			return;
		}
		if (ability.cast_check)
		{
			if (!ability.cast_check(main_player.character))
				return;
		}
		select_mode = ability.target_type;
		if (select_mode == TargetNull)
			main_player.character->cast_ability(ins, vec3(0.f), nullptr);
		else
		{
			if (ability.target_type & TargetLocation)
			{
				select_location_callback = [this](const vec3& location) {
					new CommandCastAbilityToLocation(main_player.character, ins, location);
				};
				select_distance = ability.distance;
			}
			if (ability.target_type & TargetEnemy)
			{
				select_enemy_callback = [this](cCharacterPtr character) {
					new CommandCastAbilityToTarget(main_player.character, ins, character);
				};
				select_distance = ability.distance;
			}
		}
	}
};

std::unique_ptr<Shortcut> shortcuts[10];

cCharacterPtr focus_character = nullptr;

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
		main_player.character->gain_ability(Ability::find("Fire Thrower"));
		main_player.character->gain_ability(Ability::find("Shield Bash"));
		main_player.character->gain_ability(Ability::find("Flame Weapon"));

		for (auto i = 0; i < countof(shortcuts); i++)
		{
			auto shortcut = new Shortcut;
			shortcut->key = KeyboardKey(Keyboard_1 + i);
			shortcuts[i].reset(shortcut);
		}
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
			add_chest(player1_coord + vec3(-3.f, 0.f, 2.f), Item::find("Straight Sword"));
			add_chest(player1_coord + vec3(-2.f, 0.f, 2.f), Item::find("Boots of Speed"));
			add_chest(player1_coord + vec3(-3.f, 0.f, 3.f), Item::find("Magic Candy"));
			add_chest(player1_coord + vec3(-2.f, 0.f, 3.f), Item::find("Magic Candy"));
			add_chest(player1_coord + vec3(-1.f, 0.f, 3.f), Item::find("Magic Candy"));

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

				static const wchar_t* prefabs[] = {
					L"assets\\characters\\spiderling\\main.prefab",
					L"assets\\characters\\treant\\main.prefab",
					L"assets\\characters\\boar\\main.prefab"
				};

				auto e = Entity::create();
				e->load(prefabs[linearRand(0U, (uint)countof(prefabs) - 1)]);
				e->node()->set_pos(coord);
				auto character = e->get_component_t<cCharacter>();
				character->set_faction(2);
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
		ImGui::SetNextWindowSize(ImVec2(600.f, 42.f), ImGuiCond_Always);
		ImGui::Begin("##main_panel", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
		if (main_player.character)
		{
			ImGui::BeginGroup();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.f, 1.f));
			auto dl = ImGui::GetWindowDrawList();
			const auto icon_size = 32.f;
			for (auto i = 0; i < countof(shortcuts); i++)
			{
				if (i > 0) ImGui::SameLine();
				auto pressed = ImGui::InvisibleButton(("shortcut" + str(i)).c_str(), ImVec2(icon_size, icon_size));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
				auto shortcut = shortcuts[i].get();
				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));

				shortcut->draw(dl, p0, p1);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("shortcut", &i, sizeof(int));
					ImGui::EndDragDropSource();
				}

				if (pressed)
					shortcut->click();

				if (ImGui::BeginDragDropTarget())
				{
					if (auto payload = ImGui::AcceptDragDropPayload("shortcut"); payload)
					{
						auto j = *(int*)payload->Data;
						if (i != j)
						{
							std::swap(shortcuts[i]->key, shortcuts[j]->key);
							std::swap(shortcuts[i], shortcuts[j]);
						}
					}
					if (auto payload = ImGui::AcceptDragDropPayload("ability"); payload)
					{
						auto j = *(int*)payload->Data;
						auto key = shortcut->key;
						shortcut = new AbilityShortcut(main_player.character->abilities[j].get());
						shortcut->key = key;
						shortcuts[i].reset(shortcut);
					}
					ImGui::EndDragDropTarget();
				}
				if (shortcut && shortcut->key != KeyboardKey_Count)
					dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), TypeInfo::serialize_t(shortcut->key).c_str());
			}
			ImGui::PopStyleVar();
			ImGui::EndGroup();

			ImGui::SameLine();
			ImGui::BeginGroup();
			{
				static auto img = graphics::Image::get("assets\\icons\\head.png");
				auto pressed = ImGui::InvisibleButton("btn_equipment", ImVec2(icon_size, icon_size));
				auto hovered = ImGui::IsItemHovered();
				auto active = ImGui::IsItemActive();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
				dl->AddImage(img, p0, p1);
				if (main_player.character->attribute_points > 0)
				{
					dl->AddCircleFilled(vec2(p1.x, p0.y), 7.f, ImColor(0.8f, 0.2f, 0.2f));
					dl->AddText(vec2(p1.x - 4.f, p0.y - 10.f), ImColor(1.f, 1.f, 1.f), str(main_player.character->attribute_points).c_str());
				}
				if (pressed)
				{
					if (!view_equipment.opened)
						view_equipment.open();
					else
						view_equipment.close();
				}
			}
			ImGui::SameLine();
			{
				static auto img = graphics::Image::get("assets\\icons\\book.png");
				auto pressed = ImGui::InvisibleButton("btn_ability", ImVec2(icon_size, icon_size));
				auto hovered = ImGui::IsItemHovered();
				auto active = ImGui::IsItemActive();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
				dl->AddImage(img, p0, p1);
				if (main_player.character->ability_points > 0)
				{
					dl->AddCircleFilled(vec2(p1.x, p0.y), 7.f, ImColor(0.8f, 0.2f, 0.2f));
					dl->AddText(vec2(p1.x - 4.f, p0.y - 10.f), ImColor(1.f, 1.f, 1.f), str(main_player.character->ability_points).c_str());
				}
				if (pressed)
				{
					if (!view_ability.opened)
						view_ability.open();
					else
						view_ability.close();
				}
			}
			ImGui::SameLine();
			{
				static auto img = graphics::Image::get("assets\\icons\\bag.png");
				auto pressed = ImGui::InvisibleButton("btn_bag", ImVec2(icon_size, icon_size));
				auto hovered = ImGui::IsItemHovered();
				auto active = ImGui::IsItemActive();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
				dl->AddImage(img, p0, p1);
				if (pressed)
				{
					if (!view_inventory.opened)
						view_inventory.open();
					else
						view_inventory.close();
				}
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

			for (auto i = 0; i < focus_character->buffs.size(); i++)
			{
				if (i > 0) ImGui::SameLine();
				auto& ins = focus_character->buffs[i];
				auto& buff = Buff::get(ins->id);
				ImGui::Dummy(ImVec2(16, 16));
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(buff.name.c_str());
					ImGui::Text("%d s", (int)ins->timer);
					ImGui::EndTooltip();
				}
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddImage(buff.icon_image, p0, p1, buff.icon_uvs.xy(), buff.icon_uvs.zw());
			}

			ImGui::End();
		}

		if (!tooltip.empty())
		{
			auto dl = ImGui::GetBackgroundDrawList();
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(tooltip.c_str());
			ImGui::EndTooltip();
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
		tooltip.clear();
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
			{
				hovering_chest = chest;
				tooltip = Item::get(hovering_chest->item_id).name;
			}
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
					auto dl = ImGui::GetBackgroundDrawList();
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
		for (auto i = 0; i < countof(shortcuts); i++)
		{
			auto shortcut = shortcuts[i].get();
			if (shortcut->key != KeyboardKey_Count && input->kpressed(shortcut->key))
				shortcut->click();
		}
	}

	if (main_camera.node && main_player.node)
	{
		static vec3 velocity(0.f);
		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
		main_camera.node->set_pos(smooth_damp(main_camera.node->pos, 
			main_player.node->pos + camera_length * main_camera.node->g_rot[2], velocity, 0.3f, 10000.f, delta_time));
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

void add_projectile(EntityPtr prefab, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, const std::function<void(cProjectilePtr)>& on_update)
{
	auto e = prefab->copy();
	e->node()->set_pos(pos);
	auto projectile = e->get_component_t<cProjectile>();
	projectile->set_target(target);
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);
}

void add_projectile(EntityPtr prefab, const vec3& pos, const vec3& location, float speed, float collide_radius, uint collide_faction, const std::function<void(cCharacterPtr)>& on_collide, const std::function<void(cProjectilePtr)>& on_update)
{
	auto e = prefab->copy();
	e->node()->set_pos(pos);
	auto projectile = e->get_component_t<cProjectile>();
	projectile->use_target = false;
	projectile->location = location;
	projectile->speed = speed;
	projectile->collide_radius = collide_radius;
	projectile->collide_faction = collide_faction;
	projectile->on_collide = on_collide;
	projectile->on_update = on_update;
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
