#include <flame/graphics/gui.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/volume.h>
#include <flame/universe/components/receiver.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

#include "game.h"
#include "player.h"
#include "control.h"
#include "ui/ui.h"
#include "network.h"
#include "entities/object.h"
#include "entities/character.h"
#include "entities/ability.h"
#include "entities/chest.h"

vec3			hovering_pos;
cCharacterPtr	hovering_character = nullptr;
cChestPtr		hovering_chest = nullptr;
bool			hovering_terrain = false;
cNodePtr		hovering_town;

cNodePtr										selected_target = nullptr;
TargetTypeFlags									select_mode = TargetNull;
bool											select_multiple_times = false;
std::function<void(bool, cCharacterPtr)>		select_character_callback;
std::function<void(bool, const vec3& pos)>		select_location_callback;
float											select_distance = 0.f;
float											select_range = 0.f;

void reset_select()
{
	select_mode = TargetNull;
	select_multiple_times = false;
	if (select_character_callback)
		select_character_callback(false, nullptr);
	select_character_callback = nullptr;
	if (select_location_callback)
		select_location_callback(false, vec3(0.f));
	select_location_callback = nullptr;
	select_distance = 0.f;
	select_range = 0.f;
}

void init_control()
{
	selected_target = nullptr;
}

void update_control()
{
	auto tar_ext = sRenderer::instance()->target_extent();
	if (tar_ext.x <= 0.f || tar_ext.y <= 0.f)
		return;

	auto input = sInput::instance();

	cNodePtr hovering_node = nullptr;
	if (!input->mouse_used && all(greaterThanEqual(input->mpos, vec2(0.f))) && all(lessThan(input->mpos, tar_ext)))
	{
		hovering_node = sRenderer::instance()->pick_up(input->mpos, &hovering_pos, [](cNodePtr n, DrawData& draw_data) {
			if (draw_data.categories & CateMesh)
			{
				if (auto character = n->entity->get_component_t<cCharacter>(); character)
				{
					if (auto first_child = character->entity->children.empty() ? nullptr : character->entity->children[0].get(); first_child)
					{
						for (auto& c : first_child->children)
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

				if (n->entity->name == "navmesh_model")
				{
					if (auto mesh = n->entity->get_component_t<cMesh>(); mesh)
					{
						if (mesh->instance_id != -1 && mesh->mesh_res_id != -1 && mesh->material_res_id != -1)
							draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, mesh->material_res_id);
					}
				}
				if (n->entity->name.ends_with("_town"))
				{
					if (auto mesh = n->entity->get_component_t<cMesh>(); mesh)
					{
						if (mesh->instance_id != -1 && mesh->mesh_res_id != -1 && mesh->material_res_id != -1)
							draw_data.meshes.emplace_back(mesh->instance_id, mesh->mesh_res_id, mesh->material_res_id);
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
	}

	hovering_character = nullptr;
	hovering_chest = nullptr;
	hovering_terrain = false;
	//tooltip.clear();
	if (hovering_node)
	{
		auto can_select = [](cCharacterPtr character) {
			if (!(select_mode & TargetEnemy) && player1.faction != character->faction)
				return false;
			if (!(select_mode & TargetFriendly) && player1.faction == character->faction)
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
			//tooltip = Item::get(hovering_chest->item_id).name;
		}
		if (hovering_node->entity->name == "navmesh_model" || hovering_node->entity->get_component_t<cTerrain>() || hovering_node->entity->get_component_t<cVolume>())
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
		if (hovering_node->entity->name.ends_with("_town"))
		{
			hovering_town = hovering_node;
		}
	}

	if (select_mode != TargetNull)
	{
		if (input->mpressed(Mouse_Right) || input->kpressed(Keyboard_Esc))
			reset_select();
	}
	if (input->mpressed(Mouse_Left))
	{
		if (select_mode == TargetNull)
		{
			//focus_character.set(hovering_character ? hovering_character : nullptr);
			if (hovering_character)
			{

			}
			else if (hovering_town)
			{
				selected_target = hovering_town;
			}
		}
		else
		{
			if (select_mode & TargetEnemy)
			{
				if (hovering_character && player1.faction != hovering_character->faction)
				{
					if (select_character_callback)
						select_character_callback(true, hovering_character);
					if (!select_multiple_times)
						reset_select();
				}
			}
			if (select_mode & TargetLocation)
			{
				if (hovering_terrain)
				{
					if (select_location_callback)
						select_location_callback(true, hovering_pos);
					if (!select_multiple_times)
						reset_select();
				}
			}
		}
	}
}

void command_character_idle(cCharacterPtr character)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_idle();
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "Idle"_h;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}

void command_character_hold(cCharacterPtr character)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_hold();
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "Hold"_h;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}

void command_character_moveto(cCharacterPtr character, const vec3& pos)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_move_to(pos);
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "MoveTo"_h;
		stru.t.location = pos;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}

void command_character_attack_target(cCharacterPtr character, cCharacterPtr target)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_attack_target(target);
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "AttackTarget"_h;
		stru.t.target = target->object->uid;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}

void command_character_attack_location(cCharacterPtr character, const vec3& pos)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_attack_location(pos);
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "AttackLocation"_h;
		stru.t.location = pos;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}

void command_character_pickup(cCharacterPtr character, cChestPtr chest)
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
		character->cmd_interact(chest);
	else if (multi_player == MultiPlayerAsClient)
	{
		std::ostringstream res;
		nwCommandCharacterStruct stru;
		stru.id = character->object->uid;
		stru.type = "Interact"_h;
		stru.t.target = chest->object->uid;
		pack_msg(res, nwCommandCharacter, stru);
		so_client->send(res.str());
	}
}
