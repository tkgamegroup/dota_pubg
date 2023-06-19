#include <flame/graphics/material.h>
#include <flame/universe/components/mesh.h>

#include "game.h"
#include "player.h"
#include "presets.h"
#include "entities/character.h"
#include "entities/ai.h"
#include "entities/collider.h"

void Player::init(EntityPtr _e_town)
{
	e_town = _e_town;
	if (e_town)
	{
		e_formation_grid = e_town->find_child_recursively("formation_grid");
		if (e_formation_grid)
		{
			auto x_dir = normalize(e_formation_grid->node()->pos);
			auto y_dir = cross(x_dir, vec3(0.f, 1.f, 0.f));
			auto off = -(FORMATION_CY - 1) * FORMATION_GAP * 0.5f * y_dir;
			if (auto e_grid_item = get_prefab(L"assets\\grid_cell.prefab"); e_grid_item)
			{
				for (auto y = 0; y < FORMATION_CY; y++)
				{
					for (auto x = 0; x < FORMATION_CX; x++)
					{
						auto e = e_grid_item->duplicate();
						e->name = std::format("{}, {}", x, y);
						e->node()->set_pos(vec3(x * FORMATION_GAP, 0.8f, y * FORMATION_GAP) + off);
						// the first child is the model
						e->children[0]->node()->set_scl(vec3(FORMATION_GAP));
						e_formation_grid->add_child(e);
					}
				}
			}
		}
	}

	if (auto collider = e_town->get_component_t<cCircleCollider>(); collider)
	{
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				character->die("removed"_h);
				town_hp--;
				if (town_hp == 0)
				{

				}
			}
		});
	}

	formation.resize(FORMATION_CX * FORMATION_CY);

	avaliable_unit_infos.clear();
	if (!unit_infos.infos.empty())
		avaliable_unit_infos.push_back(&unit_infos.infos[0]);
}

void Player::set_formation(uint index, UnitInfo* unit_info)
{
	if (formation[index] == unit_info)
		return;
	formation[index] = unit_info;

	auto e = e_formation_grid->children[index].get();
	if (unit_info)
	{
		auto e_unit = get_prefab(unit_info->prefab_name)->duplicate();
		e_unit->backward_traversal([](EntityPtr e) {
			std::vector<uint> comp_hashes(e->components.size());
			for (auto i = 0; i < e->components.size(); i++)
				comp_hashes[i] = e->components[i]->type_hash;
			for (auto it = comp_hashes.rbegin(); it != comp_hashes.rend(); it++)
			{
				if (*it != th<cNode>() && *it != th<cMesh>() && *it != th<cArmature>())
					e->remove_component(*it);
			}

			if (auto mesh = e->get_component_t<cMesh>(); mesh)
			{
				if (mesh->material)
				{
					auto new_material = graphics::Material::create();
					TypeInfo::get<graphics::Material>()->copy(new_material, mesh->material);
					mesh->set_material_name(L"0x" + wstr_hex((uint64)new_material));
					e->message_listeners.add([new_material](uint hash, void*, void*) {
						if (hash == "destroyed"_h)
							delete new_material;
					});
				}
			}
		});
		e->add_child(e_unit);
	}
	else
		e->remove_child(e->children[1].get()); // the first child is the grid cell model, the second is the unit preview
}

void Player::spawn_troop()
{
	troop.clear();
	
	if (e_formation_grid)
	{
		auto i = 0;
		for (auto y = 0; y < FORMATION_CY; y++)
		{
			for (auto x = 0; x < FORMATION_CX; x++)
			{
				if (auto unit_info = formation[y * FORMATION_CX + x]; unit_info)
				{
					if (auto character = add_character(unit_info->prefab_name, e_formation_grid->children[i]->node()->global_pos(), faction); character)
					{
						if (auto ai = character->entity->get_component_t<cAI>(); ai)
						{
							ai->type = UnitLaneCreep;
							ai->target_pos = troop_target_location;
						}

						character->entity->message_listeners.add([this, character](uint hash, void*, void*) {
							if (hash == "destroyed"_h)
							{
								std::erase_if(troop, [&](const auto& i) {
									return i == character;
								});
							}
						});
						troop.push_back(character);
					}
				}
				i++;
			}
		}
	}
}

void Player::remove_troop()
{
	for (auto c : troop)
		c->die("removed"_h);
	troop.clear();
}

Player player1;
Player player2;
