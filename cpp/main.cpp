#include "main.h"
#include "item.h"
#include "ability.h"
#include "buff.h"
#include "vision.h"
#include "network.h"
#include "launcher.h"
#include "object.h"
#include "character.h"
#include "spwaner.h"
#include "projectile.h"
#include "chest.h"
#include "creep_ai.h"
#include "nw_data_harvester.h"
#include "views/view_equipment.h"
#include "views/view_ability.h"
#include "views/view_inventory.h"
#include "views/view_settings.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/foundation/window.h>
#include <flame/foundation/system.h>
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

		if (terrain)
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

			site_centrality.resize(site_positions.size());
			for (auto i = 0; i < site_positions.size(); i++)
			{
				auto x = abs(site_positions[i].x * 2.f - 1.f);
				auto z = abs(site_positions[i].z * 2.f - 1.f);
				site_centrality[i] = std::make_pair(x * z, i);
			}
			std::sort(site_centrality.begin(), site_centrality.end(), [](const auto& a, const auto& b) {
				return a.first < b.first;
			});
		}
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

vec3 MainTerrain::get_coord_by_centrality(int i)
{
	if (i < 0)
		i = (int)site_centrality.size() + i;
	return get_coord(site_positions[site_centrality[i].second].xz());
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
		{
			if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
				new CommandCastAbility(main_player.character, ins);
			else if (multi_player == MultiPlayerAsClient)
			{
				std::ostringstream res;
				nwCommandCharacterStruct stru;
				stru.id = main_player.character->object->uid;
				stru.type = "CastAbility"_h;
				stru.id2 = ins->id;
				pack_msg(res, nwCommandCharacter, stru);
				so_client->send(res.str());
			}
		}
		else
		{
			if (ability.target_type & TargetLocation)
			{
				select_location_callback = [this](const vec3& location) {
					if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
						new CommandCastAbilityToLocation(main_player.character, ins, location);
					else if (multi_player == MultiPlayerAsClient)
					{
						std::ostringstream res;
						nwCommandCharacterStruct stru;
						stru.id = main_player.character->object->uid;
						stru.type = "CastAbilityToLocation"_h;
						stru.id2 = ins->id;
						stru.t.location = location;
						pack_msg(res, nwCommandCharacter, stru);
						so_client->send(res.str());
					}
				};
				select_distance = ability.distance;
			}
			if (ability.target_type & TargetEnemy)
			{
				select_enemy_callback = [this](cCharacterPtr character) {
					if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
						new CommandCastAbilityToTarget(main_player.character, ins, character);
					else if (multi_player == MultiPlayerAsClient)
					{
						std::ostringstream res;
						nwCommandCharacterStruct stru;
						stru.id = main_player.character->object->uid;
						stru.type = "CastAbilityToTarget"_h;
						stru.id2 = ins->id;
						stru.t.target = character->object->uid;
						pack_msg(res, nwCommandCharacter, stru);
						so_client->send(res.str());
					}
				};
				select_distance = ability.distance;
			}
		}
	}
};

std::unique_ptr<Shortcut> shortcuts[10];

Tracker<cCharacterPtr> focus_character;

cMain::~cMain()
{
	node->drawers.remove("main"_h);

	graphics::gui_callbacks.remove((uint)this);
	graphics::gui_cursor_callbacks.remove((uint)this);
}

static std::vector<std::pair<uint, vec3>> location_tips;
void add_location_icon(const vec3& pos)
{
	location_tips.emplace_back(30, pos);
}

struct FloatingTip
{
	uint ticks;
	vec3 pos;
	std::string text;
	vec2 size = vec2(0.f);
	cvec4 color;
};

static std::vector<FloatingTip> floating_tips;

void toggle_equipment_view()
{
	if (!view_equipment.opened)
		view_equipment.open();
	else
		view_equipment.close();
}

void toggle_ability_view()
{
	if (!view_ability.opened)
		view_ability.open();
	else
		view_ability.close();
}

void toggle_inventory_view()
{
	if (!view_inventory.opened)
		view_inventory.open();
	else
		view_inventory.close();
}

void toggle_settings_view()
{
	if (!view_settings.opened)
		view_settings.open();
	else
		view_settings.close();
}

void cMain::start()
{
	printf("main started\n");
	srand(time(0));
	root = entity;

	for (auto i = 0; i < countof(shortcuts); i++)
	{
		auto shortcut = new Shortcut;
		shortcut->key = KeyboardKey(Keyboard_1 + i);
		shortcuts[i].reset(shortcut);
	}

	init_vision();

	main_camera.init(entity->find_child("Camera"));
	main_terrain.init(entity->find_child("terrain"));

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (!main_terrain.site_positions.empty())
		{
			vec3 player1_coord;
			uint player1_faction;
			uint player1_preset_id;
			add_player(player1_coord, player1_faction, player1_preset_id);
			auto character = add_character(player1_preset_id, player1_coord, player1_faction);
			main_player.faction = player1_faction;
			main_player.character_id = character->object->uid;
			main_player.init(character->entity);
			if (auto harvester = main_player.entity->get_component_t<cNWDataHarvester>(); harvester)
			{
				//harvester->add_target("exp"_h);
				//harvester->add_target("exp_max"_h);
				//harvester->add_target("atk_type"_h);
				//harvester->add_target("atk"_h);
				//harvester->add_target("phy_def"_h);
				//harvester->add_target("mag_def"_h);
				//harvester->add_target("hp_reg"_h);
				//harvester->add_target("mp_reg"_h);
				//harvester->add_target("mov_sp"_h);
				//harvester->add_target("atk_sp"_h);
			}

			add_chest(player1_coord + vec3(-3.f, 0.f, 3.f), Item::find("Magic Candy"));
			add_chest(player1_coord + vec3(-2.f, 0.f, 3.f), Item::find("Magic Candy"));
			add_chest(player1_coord + vec3(-1.f, 0.f, 3.f), Item::find("Magic Candy"));

			//for (auto i = 1; i < main_terrain.site_centrality.size() - 1; i++)
			//{
			//	auto coord = main_terrain.get_coord_by_centrality(i);

			//	static uint preset_ids[] = {
			//		CharacterPreset::find("Life Stealer"),
			//		CharacterPreset::find("Slark")
			//	};

			//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
			//	new CommandAttackLocation(character, coord);
			//}
			//for (auto i = 0; i < 100; i++)
			//{
			//	auto coord = main_terrain.get_coord(vec2(linearRand(0.f, 1.f), linearRand(0.f, 1.f)));

			//	static uint preset_ids[] = {
			//		CharacterPreset::find("Spiderling"),
			//		CharacterPreset::find("Treant"),
			//		CharacterPreset::find("Boar")
			//	};

			//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
			//	character->entity->add_component<cCreepAI>();
			//}
		}
	}

	node->drawers.add([](DrawData& draw_data) {
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
			break;
		case PassPrimitive:
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
			break;
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
				if (pressed)
					toggle_equipment_view();
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
				if (pressed)
					toggle_ability_view();
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
					toggle_inventory_view();
			}
			ImGui::SameLine();
			{
				static auto img = graphics::Image::get("assets\\icons\\gear.png");
				auto pressed = ImGui::InvisibleButton("btn_settings", ImVec2(icon_size, icon_size));
				auto hovered = ImGui::IsItemHovered();
				auto active = ImGui::IsItemActive();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
				dl->AddImage(img, p0, p1);
				if (pressed)
					toggle_settings_view();
			}
			ImGui::EndGroup();
		}
		ImGui::End();

		auto show_buffs = [](ImDrawList* dl, cCharacterPtr character) {

			for (auto i = 0; i < character->buffs.size(); i++)
			{
				if (i > 0) ImGui::SameLine();
				auto& ins = character->buffs[i];
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
		};

		auto hp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
			ImGui::Dummy(ImVec2(width, height));
			auto p0 = (vec2)ImGui::GetItemRectMin();
			auto p1 = (vec2)ImGui::GetItemRectMax();
			dl->AddRectFilled(p0, p0 + vec2((float)character->hp / (float)character->hp_max * width, height), ImColor(0.3f, 0.7f, 0.2f));
			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
			auto str = std::format("{}/{}", character->hp / 10, character->hp_max / 10);
			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
		};
		auto mp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
			ImGui::Dummy(ImVec2(width, height));
			auto p0 = (vec2)ImGui::GetItemRectMin();
			auto p1 = (vec2)ImGui::GetItemRectMax();
			dl->AddRectFilled(p0, p0 + vec2((float)character->mp / (float)character->mp_max * width, height), ImColor(0.2f, 0.3f, 0.7f));
			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
			auto str = std::format("{}/{}", character->mp / 10, character->mp_max / 10);
			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
		};
		auto exp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
			ImGui::Dummy(ImVec2(width, height));
			auto p0 = (vec2)ImGui::GetItemRectMin();
			auto p1 = (vec2)ImGui::GetItemRectMax();
			dl->AddRectFilled(p0, p0 + vec2((float)character->exp / (float)character->exp_max * width, height), ImColor(0.7f, 0.7f, 0.2f));
			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
			auto str = std::format("LV {}  {}/{}", character->lv, character->exp, character->exp_max);
			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
		};
		auto action_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
			float time = 0.f;
			if (character->attack_timer > 0.f)
				time = character->attack_timer;
			else if (character->cast_timer > 0.f)
				time = character->cast_timer;
			if (time > 0.f)
			{
				ImGui::Dummy(ImVec2(width, height));
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((1.f - fract(time)) * width, height), ImColor(0.7f, 0.7f, 0.7f));
				dl->AddRect(p0, p1, ImColor(0.3f, 0.3f, 0.3f));
				dl->AddText(vec2((p0.x + p1.x) * 0.5f - 3.f, p0.y - 7.f), ImColor(1.f, 1.f, 1.f), str((int)time).c_str());
			}
		};

		if (main_player.character)
		{
			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(8.f, 4.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
			ImGui::SetNextWindowSize(ImVec2(200.f, 120.f), ImGuiCond_Always);
			ImGui::Begin("##main_player", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

			auto dl = ImGui::GetWindowDrawList();
			const auto bar_width = ImGui::GetContentRegionAvail().x;
			const auto bar_height = 16.f;
			hp_bar(dl, bar_width, bar_height, main_player.character);
			mp_bar(dl, bar_width, bar_height, main_player.character);
			exp_bar(dl, bar_width, bar_height, main_player.character);
			show_buffs(dl, main_player.character);
			action_bar(dl, bar_width, 4.f, main_player.character);
			auto pos = main_player.character->node->pos;
			ImGui::Text("%d, %d", (int)pos.x, (int)pos.z);

			ImGui::End();
		}

		if (focus_character.obj)
		{
			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_sz.x - 8.f, 4.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
			ImGui::SetNextWindowSize(ImVec2(100.f, 100.f), ImGuiCond_Always);
			ImGui::Begin("##focus_character", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);

			auto dl = ImGui::GetWindowDrawList();
			const auto bar_width = ImGui::GetContentRegionAvail().x;
			const auto bar_height = 16.f;
			hp_bar(dl, bar_width, bar_height, focus_character.obj);
			mp_bar(dl, bar_width, bar_height, focus_character.obj);

			show_buffs(dl, focus_character.obj);
			action_bar(dl, bar_width, 4.f, focus_character.obj);

			ImGui::End();
		}

		if (main_player.character && main_camera.camera)
		{
			for (auto character : characters)
			{
				if (character->object->visible_flags & main_player.faction)
				{
					auto p = main_camera.camera->world_to_screen(character->node->pos + vec3(0.f, character->nav_agent->height + 0.2f, 0.f));
					if (p.x > 0.f && p.y > 0.f)
					{
						p += sInput::instance()->offset;
						auto dl = ImGui::GetBackgroundDrawList();
						const auto bar_width = 80.f * (character->nav_agent->radius / 0.6f);
						const auto bar_height = 5.f;
						p.x -= bar_width * 0.5f;
						dl->AddRectFilled(p, p + vec2((float)character->hp / (float)character->hp_max * bar_width, bar_height),
							character->faction == main_player.faction ? ImColor(0.f, 1.f, 0.f) : ImColor(1.f, 0.f, 0.f));
					}
				}
			}
		}

		if (!tooltip.empty())
		{
			auto dl = ImGui::GetBackgroundDrawList();
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(tooltip.c_str());
			ImGui::EndTooltip();
		}

		if (main_camera.camera)
		{
			if (!floating_tips.empty())
			{
				for (auto& t : floating_tips)
				{
					auto p = main_camera.camera->world_to_screen(t.pos);
					if (p.x > 0.f && p.y > 0.f)
					{
						p.xy += sInput::instance()->offset;
						auto dl = ImGui::GetBackgroundDrawList();
						if (t.size.x == 0.f || t.size.y == 0.f)
							t.size = ImGui::CalcTextSize(t.text.c_str());
						dl->AddText(p - vec2(t.size.x * 0.5f, 0.f), ImColor(t.color.r, t.color.g, t.color.b, 255), t.text.c_str());
					}
					t.pos.y += 1.8f * delta_time;
				}
				for (auto it = floating_tips.begin(); it != floating_tips.end();)
				{
					it->ticks--;
					if (it->ticks == 0)
						it = floating_tips.erase(it);
					else
						it++;
				}
			}

			if (!location_tips.empty())
			{
				static graphics::ImagePtr icon_location = nullptr;
				if (!icon_location)
					icon_location = graphics::Image::get(L"assets\\icons\\location.png");
				for (auto& t : location_tips)
				{
					auto p = main_camera.camera->world_to_screen(t.second);
					if (p.x > 0.f && p.y > 0.f)
					{
						p.xy += sInput::instance()->offset;
						auto dl = ImGui::GetBackgroundDrawList();
						auto sz = (vec2)icon_location->size;
						dl->AddImage(icon_location, p - vec2(sz.x * 0.5f, sz.y), p + vec2(sz.x * 0.5f, 0.f), vec2(0.f), vec2(1.f), ImColor(1.f, 1.f, 1.f, max(0.f, t.first / 30.f)));
					}
				}
				for (auto it = location_tips.begin(); it != location_tips.end();)
				{
					it->first--;
					if (it->first == 0)
						it = location_tips.erase(it);
					else
						it++;
				}
			}
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
			auto pos = sInput::instance()->mpos + sInput::instance()->offset;
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
	switch (multi_player)
	{
	case MultiPlayerAsHost:
		nw_mtx.lock();
		if (!nw_msgs.empty())
		{
			auto p = nw_msgs.data();
			auto e = p + nw_msgs.size();
			while (p < e)
			{
				auto msg = *(uchar*)p;
				p += sizeof(uchar);
				switch (msg)
				{
				case nwCommandCharacter:
				{
					auto& stru = *(nwCommandCharacterStruct*)p;
					auto it = objects.find(stru.id);
					if (it == objects.end())
						continue;
					auto character = it->second->entity->get_component_t<cCharacter>();
					switch (stru.type)
					{
					case "Idle"_h:
						new CommandIdle(character);
						break;
					case "MoveTo"_h:
						new CommandMoveTo(character, stru.t.location);
						break;
					case "AttackTarget"_h:
					{
						auto it = objects.find(stru.t.target);
						if (it != objects.end())
							new CommandAttackTarget(character, it->second->entity->get_component_t<cCharacter>());
					}
						break;
					case "AttackLocation"_h:
						new CommandAttackLocation(character, stru.t.location);
						break;
					case "PickUp"_h:
					{
						auto it = objects.find(stru.t.target);
						if (it != objects.end())
							new CommandPickUp(character, it->second->entity->get_component_t<cChest>());
					}
						break;
					case "CastAbility"_h:
						break;
					}
					p += sizeof(nwCommandCharacterStruct);
				}
					break;
				}
			}
			nw_msgs.clear();
		}
		nw_mtx.unlock();
		break;
	case MultiPlayerAsClient:
		nw_mtx.lock();
		if (!nw_msgs.empty())
		{
			auto p = nw_msgs.data();
			auto e = p + nw_msgs.size();
			while (p < e)
			{
				auto msg = *(uchar*)p;
				p += sizeof(uchar);
				switch (msg)
				{
				case nwNewPlayerInfo:
				{
					auto& stru = *(nwNewPlayerInfoStruct*)p;
					main_player.faction = stru.faction;
					main_player.character_id = stru.character_id;
					p += sizeof(nwNewPlayerInfoStruct);
				}
					break;
				case nwAddObjects:
				{
					nwAddObjectsStruct stru;
					unserialize_binary([&](void* data, uint size) {
						memcpy(data, p, size);
						p += size;
					}, &stru);
					for (auto& item : stru.items)
					{
						if (item.preset_id < 2000)
						{
							auto character = add_character(item.preset_id - 1000, vec3(0.f, -1000.f, 0.f), 0, item.id);
							character->entity->children[0]->set_enable(false); 
							if (item.id == main_player.character_id)
								main_player.init(character->entity);
						}
						else if (item.preset_id < 3000)
						{
							auto projectle = add_projectile(item.preset_id - 2000, vec3(0.f, -1000.f, 0.f), nullptr, 0.f, nullptr, item.id);
							projectle->entity->children[0]->set_enable(false);
						}
						else
						{
							auto chest = add_chest(vec3(0.f, -1000.f, 0.f), -1, 0, item.id);
							chest->entity->children[0]->set_enable(false);
						}
					}
				}
					break;
				case nwRemoveObjects:
				{
					nwRemoveObjectsStruct stru;
					unserialize_binary([&](void* data, uint size) {
						memcpy(data, p, size);
						p += size;
					}, & stru);
					for (auto id : stru.ids)
					{
						auto it = objects.find(id);
						if (it == objects.end())
							continue;
						auto entity = it->second->entity;
						add_event([entity]() {
							entity->remove_from_parent();
							return false;
						});
					}
				}
					break;
				case nwUpdateObjects:
				{
					nwUpdateObjectsStruct stru;
					unserialize_binary([&](void* data, uint size) {
						memcpy(data, p, size);
						p += size;
					}, &stru);
					for (auto& item : stru.items)
					{
						auto it = objects.find(item.obj_id);
						if (it == objects.end())
							continue;
						auto entity = it->second->entity;
						for (auto& citem : item.comps)
						{
							auto comp = entity->components[citem.idx].get();
							auto ui = find_udt(comp->type_hash);
							auto p = citem.datas.data();
							for (auto i = 0; i < citem.names.size(); i++)
							{
								auto name = citem.names[i];
								auto vi = ui->find_variable(name); auto len = vi->type->size;
								auto dst = (char*)comp + vi->offset;
								memcpy(dst, p, len);
								p += len;

								if (name == "visible_flags"_h)
									entity->children[0]->set_enable(*(uint*)dst & main_player.faction);
							}
						}
					}
				}
					break;
				}
			}
			nw_msgs.clear();
		}
		nw_mtx.unlock();
		break;
	}

	if (main_camera.node && main_player.node)
	{
		static vec3 velocity(0.f);
		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
		main_camera.node->set_pos(smooth_damp(main_camera.node->pos,
			main_player.node->pos + camera_length * main_camera.node->g_rot[2], velocity, 0.3f, 10000.f, delta_time));
	}

	update_vision();

	if (multi_player == MultiPlayerAsHost)
	{
		if (!nw_new_players.empty())
		{
			for (auto so_id : nw_new_players)
			{
				vec3 pos;
				uint faction;
				uint preset_id;
				add_player(pos, faction, preset_id);
				auto character = add_character(preset_id, pos, faction);

				nw_players[faction].push_back(so_id);

				std::ostringstream res;
				{
					nwNewPlayerInfoStruct stru;
					stru.faction = faction;
					stru.character_id = character->object->uid;
					pack_msg(res, nwNewPlayerInfo, stru);
				}
				nwAddObjectsStruct stru;
				for (auto& pair : objects)
				{
					auto& item = stru.items.emplace_back();
					item.preset_id = pair.second->preset_id;
					item.id = pair.second->uid;
				}
				pack_msg(res, nwAddObjects, stru);
				so_server->send(so_id, res.str());
			}
		}
		{	// info new and removed objects
			std::ostringstream res;
			if (!new_objects.empty())
			{
				nwAddObjectsStruct stru;
				for (auto& o : new_objects)
				{
					auto& item = stru.items.emplace_back();
					item.preset_id = o.first;
					item.id = o.second;
				}
				pack_msg(res, nwAddObjects, stru);
				new_objects.clear();
			}
			if (!removed_objects.empty())
			{
				nwRemoveObjectsStruct stru;
				for (auto id : removed_objects)
					stru.ids.push_back(id);
				pack_msg(res, nwRemoveObjects, stru);
				removed_objects.clear();
			}
			if (auto str = res.str(); !str.empty())
			{
				for (auto& f : nw_players)
				{
					for (auto so_id : f.second)
					{
						// new players has been infoed above
						if (std::find(nw_new_players.begin(), nw_new_players.end(), so_id) == nw_new_players.end())
							so_server->send(so_id, str);
					}
				}
			}
		}
		for (auto& f : nw_players)
		{
			std::ostringstream res;
			nwUpdateObjectsStruct stru_update;
			for (auto& pair : objects)
			{
				auto entity = pair.second->entity;
				auto harvester = entity->get_component_t<cNWDataHarvester>();
				if (!harvester) continue;
				auto has_vision = get_vision(f.first, entity->node()->pos);
				nwUpdateObjectsStruct::Item item;
				item.obj_id = pair.first;
				for (auto i = 0; i < harvester->targets.size(); i++)
				{
					nwUpdateObjectsStruct::Comp citem;
					citem.idx = i;

					auto comp = entity->components[i].get();
					auto ui = find_udt(comp->type_hash);
					for (auto& pair : harvester->targets[i])
					{
						if (!has_vision && pair.first != "visible_flags"_h)
							continue;
						if (pair.second.first & f.first)
						{
							auto vi = ui->find_variable(pair.first); auto len = vi->type->size;
							citem.names.push_back(pair.first);
							auto sz = (uint)citem.datas.size();
							citem.datas.resize(sz + len);
							memcpy(citem.datas.data() + sz, (char*)comp + vi->offset, len);
							pair.second.first &= ~f.first;
						}
					}
					if (!citem.names.empty())
						item.comps.push_back(std::move(citem));
				}
				if (!item.comps.empty())
					stru_update.items.push_back(std::move(item));
			}
			if (!stru_update.items.empty())
				pack_msg(res, nwUpdateObjects, stru_update);
			if (auto str = res.str(); !str.empty())
			{
				for (auto so_id : f.second)
					so_server->send(so_id, str);
			}
		}
		nw_new_players.clear();
	}

	if (!graphics::gui_want_mouse())
	{
		auto input = sInput::instance();
		vec3 hovering_pos;
		auto hovering_node = sRenderer::instance()->pick_up(input->mpos, &hovering_pos, [](cNodePtr n, DrawData& draw_data) {
			if (draw_data.categories & CateMesh)
			{
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
			}
			if (draw_data.categories & CateTerrain)
			{
				if (auto terrain = n->entity->get_component_t<cTerrain>(); terrain)
				{
					if (terrain->instance_id != -1 && terrain->material_res_id != -1)
						draw_data.terrains.emplace_back(terrain->instance_id, product(terrain->blocks), terrain->material_res_id);
				}
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

		if (input->mpressed(Mouse_Left))
		{
			if (select_mode == TargetNull)
			{
				focus_character.set(hovering_character ? hovering_character : nullptr);
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
		if (main_player.character)
		{
			if (input->mpressed(Mouse_Right))
			{
				if (select_mode == TargetNull)
				{
					if (hovering_character)
					{
						if (hovering_character->faction != main_player.character->faction)
						{
							if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
								new CommandAttackTarget(main_player.character, hovering_character);
							else if (multi_player == MultiPlayerAsClient)
							{
								std::ostringstream res;
								nwCommandCharacterStruct stru;
								stru.id = main_player.character->object->uid;
								stru.type = "AttackTarget"_h;
								stru.t.target = hovering_character->object->uid;
								pack_msg(res, nwCommandCharacter, stru);
								so_client->send(res.str());
							}
						}
					}
					else if (hovering_chest)
					{
						if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
							new CommandPickUp(main_player.character, hovering_chest);
						else if (multi_player == MultiPlayerAsClient)
						{
							std::ostringstream res;
							nwCommandCharacterStruct stru;
							stru.id = main_player.character->object->uid;
							stru.type = "PickUp"_h;
							stru.t.target = hovering_chest->object->uid;
							pack_msg(res, nwCommandCharacter, stru);
							so_client->send(res.str());
						}
					}
					else if (hovering_terrain)
					{
						if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
							new CommandMoveTo(main_player.character, hovering_pos);
						else if (multi_player == MultiPlayerAsClient)
						{
							std::ostringstream res;
							nwCommandCharacterStruct stru;
							stru.id = main_player.character->object->uid;
							stru.type = "MoveTo"_h;
							stru.t.location = hovering_pos;
							pack_msg(res, nwCommandCharacter, stru);
							so_client->send(res.str());
						}
						add_location_icon(hovering_pos);
					}
				}
				else
					reset_select();
			}

			if (input->kpressed(Keyboard_A))
			{
				select_mode = TargetType(TargetEnemy | TargetLocation);
				select_enemy_callback = [](cCharacterPtr character) {
					if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
						new CommandAttackTarget(main_player.character, character);
					else if (multi_player == MultiPlayerAsClient)
					{
						std::ostringstream res;
						nwCommandCharacterStruct stru;
						stru.id = main_player.character->object->uid;
						stru.type = "AttackTarget"_h;
						stru.t.target = character->object->uid;
						pack_msg(res, nwCommandCharacter, stru);
						so_client->send(res.str());
					}
				};
				select_location_callback = [](const vec3& pos) {
					if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
						new CommandAttackLocation(main_player.character, pos);
					else if (multi_player == MultiPlayerAsClient)
					{
						std::ostringstream res;
						nwCommandCharacterStruct stru;
						stru.id = main_player.character->object->uid;
						stru.type = "AttackLocation"_h;
						stru.t.location = pos;
						pack_msg(res, nwCommandCharacter, stru);
						so_client->send(res.str());
					}
				};
			}
			for (auto i = 0; i < countof(shortcuts); i++)
			{
				auto shortcut = shortcuts[i].get();
				if (shortcut->key != KeyboardKey_Count && input->kpressed(shortcut->key))
					shortcut->click();
			}
		}
		if (input->kpressed(Keyboard_Esc))
			reset_select();
		if (input->kpressed(Keyboard_F1))
			toggle_equipment_view();
		if (input->kpressed(Keyboard_F2))
			toggle_ability_view();
		if (input->kpressed(Keyboard_F3))
			toggle_inventory_view();
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

static std::map<std::filesystem::path, EntityPtr> prefabs;

EntityPtr get_prefab(const std::filesystem::path& _path)
{
	auto path = Path::get(_path);
	auto it = prefabs.find(path);
	if (it == prefabs.end())
	{
		auto e = Entity::create();
		e->load(path);
		it = prefabs.emplace(path, e).first;
	}
	return it->second;
}

void add_player(vec3& pos, uint& faction, uint& preset_id)
{
	static uint idx = 0;
	pos = main_terrain.get_coord_by_centrality(-idx - 1);
	faction =  1 << (log2i((uint)FactionParty1) + idx);
	preset_id = CharacterPreset::find("Dragon Knight");
	idx++;
}

std::vector<cCharacterPtr> find_characters(const vec3& pos, float radius, uint faction)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos.xz(), radius, objs, CharacterTag);
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

cCharacterPtr add_character(uint preset_id, const vec3& pos, uint faction, uint id)
{
	auto& preset = CharacterPreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(1000 + preset_id, id);
	auto character = e->get_component_t<cCharacter>();
	character->preset = &preset;
	character->set_faction(faction);
	if (multi_player == MultiPlayerAsHost)
	{
		auto harvester = e->add_component<cNWDataHarvester>();
		harvester->add_target(th<cCharacter>(), "hp"_h);
		harvester->add_target(th<cCharacter>(), "hp_max"_h);
		harvester->add_target(th<cCharacter>(), "mp"_h);
		harvester->add_target(th<cCharacter>(), "mp_max"_h);
		harvester->add_target(th<cCharacter>(), "lv"_h);
	}
	root->add_child(e);

	return character;
}

cProjectilePtr add_projectile(uint preset_id, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id)
{
	auto& preset = ProjectilePreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(2000 + preset_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectile->preset_id = preset_id;
	projectile->target.set(target);
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);

	return projectile;
}

cProjectilePtr add_projectile(uint preset_id, const vec3& pos, const vec3& location, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id)
{
	auto& preset = ProjectilePreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(2000 + preset_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectile->preset_id = preset_id;
	projectile->use_target = false;
	projectile->location = location;
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);

	return projectile;
}

cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num, uint id)
{
	auto e = get_prefab(L"assets\\models\\chest.prefab")->copy();
	e->node()->set_pos(main_terrain.get_coord(pos));
	root->add_child(e);
	auto object = e->get_component_t<cObject>();
	object->init(3000, id);
	auto chest = e->get_component_t<cChest>();
	chest->item_id = item_id;
	chest->item_num = item_num;

	return chest;
}

void teleport(cCharacterPtr character, const vec3& location)
{
	character->node->set_pos(location);
	character->nav_agent->update_pos();
}

void add_floating_tip(const vec3& pos, const std::string& text, const cvec4& color)
{
	auto& t = floating_tips.emplace_back();
	t.ticks = 30;
	t.pos = pos;
	t.text = text;
	t.color = color;
}

EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cLauncher::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cMain::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cObject::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cSpwaner::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cProjectile::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cChest::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCreepAI::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cNWDataHarvester::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}
