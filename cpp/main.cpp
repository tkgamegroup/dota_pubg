#include "main.h"
#include "item.h"
#include "ability.h"
#include "buff.h"
#include "vision.h"
#include "network.h"
#include "launcher.h"
#include "object.h"
#include "character.h"
#include "projectile.h"
#include "effect.h"
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
#include <flame/universe/world.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/volume.h>
#include <flame/universe/components/particle_system.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/nav_scene.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

std::string get_show_name(const std::string& name)
{
	auto ret = name;
	ret[0] = toupper(ret[0]);
	for (auto i = 0; i < ret.size() - 1; i++)
	{
		if (ret[i] == '_')
		{
			ret[i] = ' ';
			ret[i + 1] = toupper(ret[i + 1]);
		}
	}
	return ret;
}

bool parse_literal(const std::string& str, int& id)
{
	if (SUS::match_head_tail(str, "\"", "\"h"))
	{
		id = sh(str.substr(1, str.size() - 3).c_str());
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"s"))
	{
		CharacterState state;
		TypeInfo::unserialize_t(str.substr(1, str.size() - 3), state);
		id = state;
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"b"))
	{
		id = Buff::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"i"))
	{
		id = Item::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"a"))
	{
		id = Ability::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"t"))
	{
		id = Talent::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"ef"))
	{
		id = EffectPreset::find(str.substr(1, str.size() - 4));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"pt"))
	{
		id = ProjectilePreset::find(str.substr(1, str.size() - 4));
		return true;
	}
	return false;
}

bool in_editor = false;
Entity** editor_p_selecting_entity = nullptr;
bool* editor_p_control = nullptr;

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
		hf_terrain = e->get_component_t<cTerrain>();
		mc_terrain = e->get_component_t<cVolume>();

		if (hf_terrain)
		{
			extent = hf_terrain->extent;

			if (auto height_map_info_fn = hf_terrain->height_map->filename; !height_map_info_fn.empty())
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
		else if (mc_terrain)
		{
			extent = mc_terrain->extent;
		}
	}
}

vec3 MainTerrain::get_coord(const vec2& uv)
{
	if (hf_terrain)
		return node->pos + extent * vec3(uv.x, 1.f - hf_terrain->height_map->linear_sample(uv).r, uv.y);
	else if (mc_terrain)
		return node->pos + extent * vec3(uv.x, 1.f - mc_terrain->height_map->linear_sample(uv).r, uv.y);
	return vec3(0.f);
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

		character->message_listeners.add([](CharacterMessage msg, sVariant p0, sVariant p1, sVariant p2, sVariant p3) {
			auto find_shortcut = [](Shortcut::Type type, int id) {
				for (auto& shortcut : shortcuts)
				{
					if (shortcut->type == type && shortcut->id == id)
						return true;
				}
				return false;
			};

			switch (msg)
			{
			case CharacterGainItem:
			{
				auto ins = main_player.character->inventory[p2.i].get();
				if (Item::get(ins->id).active && !find_shortcut(Shortcut::tItem, ins->id))
				{
					for (auto& shortcut : shortcuts)
					{
						if (shortcut->type == Shortcut::tNull)
						{
							auto key = shortcut->key;
							shortcut.reset(new ItemShortcut(ins));
							shortcut->key = key;
							break;
						}
					}
				}
			}
				break;
			case CharacterGainAbility:
			{
				auto ins = main_player.character->abilities[p2.i].get();
				if (ins->lv > 0 && Ability::get(ins->id).active && !find_shortcut(Shortcut::tAbility, ins->id))
				{
					for (auto& shortcut : shortcuts)
					{
						if (shortcut->type == Shortcut::tNull)
						{
							auto key = shortcut->key;
							shortcut.reset(new AbilityShortcut(ins));
							shortcut->key = key;
							break;
						}
					}
				}
			}
				break;
			case CharacterAbilityLevelUp:
			{
				auto ins = main_player.character->abilities[p0.i].get();
				if (ins->lv == 1 && Ability::get(ins->id).active && !find_shortcut(Shortcut::tAbility, ins->id))
				{
					for (auto& shortcut : shortcuts)
					{
						if (shortcut->type == Shortcut::tNull)
						{
							auto key = shortcut->key;
							shortcut.reset(new AbilityShortcut(ins));
							shortcut->key = key;
							break;
						}
					}
				}
			}
				break;
			}
		});
	}
}

MainCamera main_camera;
MainTerrain main_terrain;
MainPlayer main_player;

vec3			hovering_pos;
cCharacterPtr	hovering_character = nullptr;
cChestPtr		hovering_chest = nullptr;
bool			hovering_terrain = false;

TargetType								select_mode = TargetNull;
std::function<void(cCharacterPtr)>		select_enemy_callback;
std::function<void(const vec3& pos)>	select_location_callback;
float									select_distance = 0.f;
float									select_range = 0.f;
float									select_angle = 0.f;
float									select_start_radius = 0.f;
void reset_select()
{
	select_mode = TargetNull;
	select_enemy_callback = nullptr;
	select_location_callback = nullptr;
	select_distance = 0.f;
	select_range = 0.f;
	select_angle = 0.f;
	select_start_radius = 0.f;
}

std::string illegal_op_str;
float illegal_op_str_timer = 0.f;

std::string tooltip;

ItemShortcut::ItemShortcut(ItemInstance* ins) :
	ins(ins)
{
	type = tItem;
	id = ins->id;
}

void ItemShortcut::draw(ImDrawList* dl, const vec2& p0, const vec2& p1)
{
	auto& item = Item::get(ins->id);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(item.name.c_str());
		ImGui::TextUnformatted(item.description.c_str());
		ImGui::EndTooltip();
	}
	dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());
}

void ItemShortcut::click()
{
	main_player.character->use_item(ins);
}

AbilityShortcut::AbilityShortcut(AbilityInstance* ins) :
	ins(ins)
{
	type = tAbility;
	id = ins->id;
}

void AbilityShortcut::draw(ImDrawList* dl, const vec2& p0, const vec2& p1)
{
	auto& ability = Ability::get(ins->id);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(ability.name.c_str());
		ImGui::TextUnformatted(ability.description.c_str());
		ImGui::EndTooltip();
	}
	dl->AddImage(ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());

	if (ins->cd_max > 0.f && ins->cd_timer > 0.f)
	{
		dl->PushClipRect(p0, p1);
		auto c = (p0 + p1) * 0.5f;
		dl->PathLineTo(c);
		dl->PathArcTo(c, p1.x - p0.x, ((ins->cd_timer / ins->cd_max) * 2.f - 0.5f) * glm::pi<float>(), -0.5f * glm::pi<float>());
		dl->PathFillConvex(ImColor(0.f, 0.f, 0.f, 0.5f));
		dl->AddText(p0 + vec2(8.f), ImColor(1.f, 1.f, 1.f, 1.f), std::format("{:.1f}", ins->cd_timer).c_str());
		dl->PopClipRect();
	}
}

void AbilityShortcut::click()
{
	if (ins->cd_timer > 0.f)
	{
		illegal_op_str = "Cooldowning.";
		illegal_op_str_timer = 3.f;
		return;
	}
	auto& ability = Ability::get(ins->id);
	if (main_player.character->mp < ability.get_mp(ins->lv))
	{
		illegal_op_str = "Not Enough MP.";
		illegal_op_str_timer = 3.f;
		return;
	}
	select_mode = ability.target_type;
	if (select_mode == TargetNull)
	{
		if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
			new CharacterCommandCastAbility(main_player.character, ins);
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
					new CharacterCommandCastAbilityToLocation(main_player.character, ins, location);
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
			select_distance = ability.get_distance(ins->lv);
			select_range = ability.get_range(ins->lv);
			select_angle = ability.angle;
			select_start_radius = ability.start_radius;
		}
		if (ability.target_type & TargetEnemy)
		{
			select_enemy_callback = [this](cCharacterPtr character) {
				if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
					new CharacterCommandCastAbilityToTarget(main_player.character, ins, character);
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
			select_distance = ability.get_distance(ins->lv);
		}
	}
}

std::unique_ptr<Shortcut> shortcuts[10];

Tracker<cCharacterPtr> focus_character;

cMain::~cMain()
{
	deinit_vision();
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

float gtime = -1.f;

struct MonsterSpawnningRule
{
	uint preset_id;
	float delay;
	float number_function_factor_a;
	float number_function_factor_b;
	float number_function_factor_c;

	uint spawnned_numbers = 0;
};
std::vector<MonsterSpawnningRule> monster_spawnning_rules;

void init_spawnning_rules()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\monster_spawnnings.ini")).sections)
	{
		auto preset_id = CharacterPreset::find(section.name);
		if (preset_id != -1)
		{
			auto& rule = monster_spawnning_rules.emplace_back();
			rule.preset_id = preset_id;
			for (auto& e : section.entries)
			{
				switch (e.key_hash)
				{
				case "delay"_h:
					rule.delay = s2t<float>(e.values[0]);
					break;
				case "number_function_factor_a"_h:
					rule.number_function_factor_a = s2t<float>(e.values[0]);
					break;
				case "number_function_factor_b"_h:
					rule.number_function_factor_b = s2t<float>(e.values[0]);
					break;
				case "number_function_factor_c"_h:
					rule.number_function_factor_c = s2t<float>(e.values[0]);
					break;
				}
			}
		}
	}
}

void cMain::on_active()
{
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
			if (main_player.character)
			{
				auto r = main_player.nav_agent->radius;
				auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
				std::vector<vec3> pts(circle_draw.pts.size() * 2);
				auto center = main_player.node->pos;
				for (auto i = 0; i < circle_draw.pts.size(); i++)
				{
					pts[i * 2 + 0] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
					pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
				}
				draw_data.primitives.emplace_back("LineList"_h, std::move(pts), cvec4(255, 255, 255, 255));
			}
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
		}, "main"_h);

	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		auto tar_ext = sRenderer::instance()->target_extent();
	if (tar_ext.x <= 0.f || tar_ext.y <= 0.f)
		return;

	if (!editor_p_control || !*editor_p_control)
	{
		if (!graphics::gui_want_mouse())
		{
			auto input = sInput::instance();
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
					if (terrain->instance_id != -1)
						draw_data.terrains.emplace_back(terrain->instance_id, terrain->blocks, terrain->material_res_id);
				}
			}
			if (draw_data.categories & CateMarchingCubes)
			{
				if (auto volume = n->entity->get_component_t<cVolume>(); volume)
				{
					if (volume->marching_cubes)
					{
						if (volume->instance_id != -1)
							draw_data.volumes.emplace_back(volume->instance_id, volume->blocks, volume->material_res_id);
					}
				}
			}
			if (sInput::instance()->kpressed(Keyboard_F12))
				draw_data.graphics_debug = true;
				});
			hovering_character = nullptr;
			hovering_chest = nullptr;
			hovering_terrain = false;
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
				sScene::instance()->navmesh_nearest_point(hovering_pos, vec3(2.f, 4.f, 2.f), hovering_pos);
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
				if (hovering_node->entity->get_component_t<cTerrain>() || hovering_node->entity->get_component_t<cVolume>())
				{
					hovering_terrain = true;

					if ((select_mode & TargetLocation) == 0)
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
									new CharacterCommandAttackTarget(main_player.character, hovering_character);
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
								new CharacterCommandPickUp(main_player.character, hovering_chest);
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
								new CharacterCommandMoveTo(main_player.character, hovering_pos);
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
							new CharacterCommandAttackTarget(main_player.character, character);
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
							new CharacterCommandAttackLocation(main_player.character, pos);
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
				if (input->kpressed(Keyboard_S))
				{
					new CharacterCommandIdle(main_player.character);
				}
				if (input->kpressed(Keyboard_H))
				{
					new CharacterCommandHold(main_player.character);
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

	if (true || in_editor)
	{
		ImGui::Begin("Status");

		auto character_status = [](cCharacterPtr character) {
			ImGui::Text("Name: %s", character->entity->name.c_str());

			auto& p = character->node->pos;
			ImGui::Text("  pos: %.2f, %.2f, %.2f", p.x, p.y, p.z);
			auto& tp = character->nav_agent->target_pos;
			auto& td = character->nav_agent->dist;
			ImGui::Text("  tpos: %.2f, %.2f, %.2f (%.2f)", tp.x, tp.y, tp.z, td);
			auto& ta = character->nav_agent->ang_diff;
			ImGui::Text("  tang: %.2f", ta);

			const char* action_name;
			switch (character->action)
			{
			case ActionNone:
				action_name = "None";
				break;
			case ActionMove:
				action_name = "Move";
				break;
			case ActionAttack:
				action_name = "Attack";
				break;
			case ActionCast:
				action_name = "Cast";
				break;
			}
			ImGui::Text("  action: %s", action_name);
		};

		ImGui::TextUnformatted("Main player:");
		if (main_player.character)
			character_status(main_player.character);
		ImGui::TextUnformatted("Focus:");
		if (focus_character.obj)
			character_status(focus_character.obj);

		if (ImGui::Button("Set 'selecting entity' As Main Player"))
		{
			if (editor_p_selecting_entity && *editor_p_selecting_entity)
				main_player.init(*editor_p_selecting_entity);
		}

		ImGui::End();
	}

	ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_ext.x * 0.5f, tar_ext.y), ImGuiCond_Always, ImVec2(0.5f, 1.f));
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
			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));

			auto shortcut = shortcuts[i].get();
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
		auto str = std::format("{}/{}", character->hp, character->hp_max);
		auto text_width = ImGui::CalcTextSize(str.c_str()).x;
		dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
	};
	auto mp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
		ImGui::Dummy(ImVec2(width, height));
		auto p0 = (vec2)ImGui::GetItemRectMin();
		auto p1 = (vec2)ImGui::GetItemRectMax();
		dl->AddRectFilled(p0, p0 + vec2((float)character->mp / (float)character->mp_max * width, height), ImColor(0.2f, 0.3f, 0.7f));
		dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
		auto str = std::format("{}/{}", character->mp, character->mp_max);
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
		ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_ext.x - 8.f, 4.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
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
			//if (character->object->visible_flags & main_player.faction)
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
					auto ext = (vec2)icon_location->extent;
					dl->AddImage(icon_location, p - vec2(ext.x * 0.5f, ext.y), p + vec2(ext.x * 0.5f, 0.f), vec2(0.f), vec2(1.f), ImColor(1.f, 1.f, 1.f, max(0.f, t.first / 30.f)));
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
		auto p = vec2((tar_ext.x - text_size.x) * 0.5f, tar_ext.y - 160.f - text_size.y);
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
		auto tiles = vec2(icon_cursors->tiles);
		int cursor_x = 0, cursor_y = 0;
		if (select_mode != TargetNull)
		{
			cursor_x = 3;
			cursor_y = 0;
		}
		auto pos = sInput::instance()->mpos + sInput::instance()->offset;
		auto dl = ImGui::GetForegroundDrawList();
		dl->AddImage(icon_cursors, pos + vec2(-32.f), pos + vec2(32.f),
			vec2(cursor_x, cursor_y) / tiles,
			vec2(cursor_x + 1, cursor_y + 1) / tiles);
	}
		}, (uint)this);
	graphics::gui_cursor_callbacks.add([this](CursorType cursor) {
		auto mpos = sInput::instance()->mpos;
	auto screen_ext = sRenderer::instance()->target_extent();
	if (mpos.x < 0.f || mpos.x > screen_ext.x || mpos.y < 0.f || mpos.y > screen_ext.y)
		return cursor;
	return CursorNone;
		}, (uint)this);
}

void cMain::on_inactive()
{
	node->drawers.remove("main"_h);

	graphics::gui_callbacks.remove((uint)this);
	graphics::gui_cursor_callbacks.remove((uint)this);
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
	 
	init_effects();
	init_projectiles();
	init_items();
	init_buffs();
	init_abilities();
	init_characters();
	init_spawnning_rules();
	for (auto& preset : effect_presets)
	{
		get_prefab(preset.path)->forward_traversal([](EntityPtr e) {
			if (auto particle_system = e->get_component_t<cParticleSystem>(); particle_system)
				sRenderer::instance()->get_material_res(particle_system->material);
		});
	}

	if (auto nav_scene = entity->get_component_t<cNavScene>(); nav_scene)
	{
		nav_scene->finished_callback.add([this]() {
			if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
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

				if (main_terrain.hf_terrain && !main_terrain.site_positions.empty())
				{
					add_chest(player1_coord + vec3(-3.f, 0.f, 3.f), Item::find("Magic Candy"));
					add_chest(player1_coord + vec3(-2.f, 0.f, 3.f), Item::find("Magic Candy"));
					add_chest(player1_coord + vec3(-1.f, 0.f, 3.f), Item::find("Magic Candy"));

					//for (auto i = 1; i < main_terrain.site_centrality.size() - 1; i++)
					//{
					//	auto coord = main_terrain.get_coord_by_centrality(i);

						//static uint preset_ids[] = {
						//	CharacterPreset::find("Life Stealer"),
						//	CharacterPreset::find("Slark")
						//};

					//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
					//	new CharacterCommandAttackLocation(character, coord);
					//}
					//for (auto i = 0; i < 100; i++)
					//{
					//	auto coord = main_terrain.get_coord(vec2(linearRand(0.f, 1.f), linearRand(0.f, 1.f)));

						//static uint preset_ids[] = {
						//	CharacterPreset::find("Spiderling"),
						//	CharacterPreset::find("Treant"),
						//	CharacterPreset::find("Boar")
						//};

					//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
					//	character->entity->add_component<cCreepAI>();
					//}
				}

				gtime = 0.f;
			}
		});
	}
}

void cMain::update()
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		for (auto it = cl_threads.begin(); it != cl_threads.end();)
		{
			auto& thread = *it;
			if (thread.wait_timer > 0.f)
				thread.wait_timer -= delta_time;
			else
			{
				while (!thread.frames.empty() && thread.wait_timer <= 0.f)
					thread.execute();
				if (thread.frames.empty())
					it = cl_threads.erase(it);
				else
					it++;
			}
		}

		removing_dead_effects = true;
		for (auto o : dead_effects)
			o->entity->remove_from_parent();
		removing_dead_effects = false;

		removing_dead_projectiles = true;
		for (auto o : dead_projectiles)
			o->entity->remove_from_parent();
		removing_dead_projectiles = false;

		removing_dead_chests = true;
		for (auto o : dead_chests)
		{
			if (hovering_chest == o)
				hovering_chest = nullptr;
			o->entity->remove_from_parent();
		}
		removing_dead_chests = false;

		removing_dead_characters = true;
		for (auto o : dead_characters)
		{
			if (hovering_character == o)
				hovering_character = nullptr;
			o->entity->remove_from_parent();
		}
		removing_dead_characters = false;

		dead_effects.clear();
		dead_projectiles.clear();
		dead_chests.clear();
		dead_characters.clear();

		if (main_player.character)
		{
			if (main_player.character->command && main_player.character->command->type == "Idle"_h)
				new CharacterCommandHold(main_player.character);
		}
	}

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
						new CharacterCommandIdle(character);
						break;
					case "MoveTo"_h:
						new CharacterCommandMoveTo(character, stru.t.location);
						break;
					case "AttackTarget"_h:
					{
						auto it = objects.find(stru.t.target);
						if (it != objects.end())
							new CharacterCommandAttackTarget(character, it->second->entity->get_component_t<cCharacter>());
					}
						break;
					case "AttackLocation"_h:
						new CharacterCommandAttackLocation(character, stru.t.location);
						break;
					case "PickUp"_h:
					{
						auto it = objects.find(stru.t.target);
						if (it != objects.end())
							new CharacterCommandPickUp(character, it->second->entity->get_component_t<cChest>());
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
						else if (item.preset_id < 4000)
						{
							auto projectle = add_projectile(item.preset_id - 3000, vec3(0.f, -1000.f, 0.f), nullptr, 0.f, nullptr, item.id);
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

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (main_player.character)
		{
			if (gtime >= 0.f)
			{
				gtime += delta_time;
				for (auto& rule : monster_spawnning_rules)
				{
					auto t = gtime / 60.f;
					t -= rule.delay;
					if (t < 0.f)
						continue;

					auto n = rule.number_function_factor_a * t + rule.number_function_factor_b * t * t + rule.number_function_factor_c;
					n -= rule.spawnned_numbers;

					for (auto i = 0; i < n; i++)
					{
						auto uv = (main_player.node->pos.xz() + circularRand(20.f)) / main_terrain.extent.xz();
						if (uv.x > 0.f && uv.x < 1.f && uv.y > 0.f && uv.y < 1.f)
						{
							auto pos = main_terrain.get_coord(uv);
							if (pos.y > main_terrain.node->pos.y + 3.f)
							{
								auto path = sScene::instance()->query_navmesh_path(pos, main_player.node->pos, 0);
								if (path.size() >= 2 && distance(path.back(), main_player.node->pos) < 0.3f)
								{
									auto character = add_character(rule.preset_id, pos, FactionCreep);
									character->add_buff(Buff::find("Cursed"), -1.f, uint(gtime / 60.f) + 1);
									new CharacterCommandAttackTarget(character, main_player.character);

									rule.spawnned_numbers++;
								}
							}
						}
					}
				}
			}
		}
	}

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

void enable_game(bool v)
{
	root->world->update_components = v;
	sScene::instance()->enable = v;
}

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
	if (!main_terrain.site_centrality.empty())
		pos = main_terrain.get_coord_by_centrality(-idx - 1);
	else
		pos = main_terrain.get_coord(vec2(0.5f));
	faction =  1 << (log2i((uint)FactionParty1) + idx);
	preset_id = CharacterPreset::find("Dragon Knight");
	idx++;
}

std::vector<cCharacterPtr> find_characters(uint faction, const vec3& pos, float r1, float r0, float central_angle, float direction_angle)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos.xz(), r1, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
		{
			if (central_angle == 360.f || 
				circle_sector_intersect(obj->pos, chr->nav_agent->radius, pos, r0, r1, central_angle, direction_angle))
				ret.push_back(chr);
		}
	}

	// sort them by distances
	std::vector<std::pair<float, cCharacterPtr>> distance_list(ret.size());
	for (auto i = 0; i < ret.size(); i++)
	{
		auto c = ret[i];
		distance_list[i] = std::make_pair(distance(c->node->pos, pos), c);
	}
	std::sort(distance_list.begin(), distance_list.end(), [](const auto& a, const auto& b) {
		return a.first < b.first;
	});
	for (auto i = 0; i < ret.size(); i++)
		ret[i] = distance_list[i].second;
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
	characters.push_back(character);
	character->preset = &preset;
	character->set_faction(faction);
	if (multi_player == MultiPlayerAsHost)
	{
		auto harvester = e->add_component_t<cNWDataHarvester>();
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
	projectiles.push_back(projectile);
	projectile->preset = &preset;
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
	projectiles.push_back(projectile);
	projectile->preset = &preset;
	projectile->use_target = false;
	projectile->location = location;
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);

	return projectile;
}

cEffectPtr add_effect(uint preset_id, const vec3& pos, const vec3& eul, float duration, uint id)
{
	auto& preset = EffectPreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	auto node = e->node();
	node->set_pos(pos);
	node->set_eul(eul);
	auto object = e->get_component_t<cObject>();
	object->init(3000 + preset_id, id);
	auto effect = e->get_component_t<cEffect>();
	effects.push_back(effect);
	effect->preset = &preset;
	effect->duration = duration;
	root->add_child(e);

	return effect;
}

cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num, uint id)
{
	auto e = get_prefab(L"assets\\models\\chest.prefab")->copy();
	e->node()->set_pos(main_terrain.get_coord(pos));
	root->add_child(e);
	auto object = e->get_component_t<cObject>();
	object->init(4000, id);
	auto chest = e->get_component_t<cChest>();
	chests.push_back(chest);
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

extern "C" EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cLauncher::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cMain::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cObject::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cProjectile::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cChest::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCreepAI::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cNWDataHarvester::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}

extern "C" EXPORT void set_editor_info(Entity** p_selecting_entity, bool* p_control)
{
	in_editor = true;
	editor_p_selecting_entity = p_selecting_entity;
	editor_p_control = p_control;
}
