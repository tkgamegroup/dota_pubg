#include <flame/graphics/canvas.h>
#include <flame/graphics/extension.h>
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
#include "../entities/ai.h"
#include "../entities/ability.h"
#include "../entities/chest.h"
#include "../entities/town.h"
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
		PanelTownConstrucion,
		PanelTownBuilding,
		PanelTownAttack
	};
	PanelType type = PanelNone;

	EntityPtr e = nullptr;
	cImagePtr avatar_image = nullptr;
	EntityPtr sub_panel1 = nullptr;
	cTextPtr name_text = nullptr;
	EntityPtr hp_bar = nullptr;
	EntityPtr mp_bar = nullptr;
	EntityPtr sub_panel2 = nullptr;
	EntityPtr character_stats_panel = nullptr;
	cTextPtr atk_text = nullptr;
	cTextPtr atk_distance_text = nullptr;
	cTextPtr atk_intterval_text = nullptr;
	cTextPtr move_speed_text = nullptr;
	EntityPtr resources_production_panel = nullptr;
	cTextPtr blood_production_text = nullptr;
	cTextPtr bones_production_text = nullptr;
	cTextPtr soul_sand_production_text = nullptr;
	EntityPtr sub_panel3 = nullptr;
	cListPtr construction_list = nullptr;
	cListPtr training_list = nullptr;
	cListPtr attack_list = nullptr;
	cListPtr action_list = nullptr;

	cCharacterPtr character = nullptr;
	TownInstance* town = nullptr;
	int building_index = -1;

	int get_last_row_idx(cListPtr list)
	{
		auto columns = list->entity->get_component<cLayout>()->columns;
		return (int)floor(((float)list->entity->children.size() - 0.5f) / columns) * columns;
	}

	void init(EntityPtr _e)
	{
		e = _e;
		if (auto e = _e->find_child("avatar_image"); e)
			avatar_image = e->get_component<cImage>();
		sub_panel1 = _e->find_child("sub_panel1");
		if (sub_panel1)
		{
			if (auto e = sub_panel1->find_child("name"); e)
				name_text = e->get_component<cText>();
			hp_bar = sub_panel1->find_child("hp_bar");
			mp_bar = sub_panel1->find_child("mp_bar");
		}
		sub_panel2 = _e->find_child("sub_panel2");
		if (sub_panel2)
		{
			character_stats_panel = sub_panel2->find_child("character_stats");
			if (character_stats_panel)
			{
				if (auto e = character_stats_panel->find_child("atk_text"); e)
					atk_text = e->get_component<cText>();
				if (auto e = character_stats_panel->find_child("atk_distance_text"); e)
					atk_distance_text = e->get_component<cText>();
				if (auto e = character_stats_panel->find_child("atk_intterval_text"); e)
					atk_intterval_text = e->get_component<cText>();
				if (auto e = character_stats_panel->find_child("move_speed_text"); e)
					move_speed_text = e->get_component<cText>();
			}
			resources_production_panel = sub_panel2->find_child("resources_production");
			if (resources_production_panel)
			{
				if (auto e = resources_production_panel->find_child("blood"); e)
					blood_production_text = e->find_child("text")->get_component<cText>();
				if (auto e = resources_production_panel->find_child("bones"); e)
					bones_production_text = e->find_child("text")->get_component<cText>();
				if (auto e = resources_production_panel->find_child("soul_sand"); e)
					soul_sand_production_text = e->find_child("text")->get_component<cText>();
			}
		}
		sub_panel3 = _e->find_child("sub_panel3");
		if (sub_panel3)
		{
			if (auto e = sub_panel3->find_child("construction_list"); e)
				construction_list = e->get_component<cList>();
			if (auto e = sub_panel3->find_child("training_list"); e)
				training_list = e->get_component<cList>();
			if (auto e = sub_panel3->find_child("attack_list"); e)
				attack_list = e->get_component<cList>();
		}
		if (auto e = _e->find_child("action_list"); e)
			action_list = e->get_component<cList>();
	}

	void reset()
	{
		type = PanelNone;
		character = nullptr;
		town = nullptr;
		building_index = -1;

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
		if (sub_panel2)
		{
			for (auto& c : sub_panel2->children)
				c->set_enable(false);
		}
		if (sub_panel3)
		{
			for (auto& c : sub_panel3->children)
			{
				if (auto list = c->get_parent_component<cList>(); list)
					list->set_count(0);
				c->set_enable(false);
			}
		}
		if (action_list)
		{
			for (auto& c : action_list->entity->children)
			{
				if (auto image = c->get_component<cImage>(); image)
					image->set_image_name(L"");
				if (auto receiver = c->get_component<cReceiver>(); receiver)
					receiver->event_listeners.clear();
				for (auto& c2 : c->children)
					c2->set_enable(false);
			}
		}

		sInput::instance()->hovering_receiver = nullptr;
	}

	void update_character()
	{
		if (hp_bar)
		{
			hp_bar->find_child("bar")->get_component<cElement>()->set_scl(vec2((float)character->hp / (float)character->hp_max, 1.f));
			hp_bar->find_child("text")->get_component<cText>()->set_text(wstr(character->hp) + L"/" + wstr(character->hp_max));
		}
		if (mp_bar)
		{
			mp_bar->find_child("bar")->get_component<cElement>()->set_scl(vec2((float)character->mp / (float)character->mp_max, 1.f));
			mp_bar->find_child("text")->get_component<cText>()->set_text(wstr(character->mp) + L"/" + wstr(character->mp_max));
		}

		if (atk_text)
			atk_text->set_text(std::format(L"ATK: {} ({})", character->atk, character->atk_type == PhysicalDamage ? L"Physical" : L"Magical"));
		if (atk_distance_text)
			atk_distance_text->set_text(std::format(L"ATK Distance: {}", character->atk_distance));
		if (atk_intterval_text)
			atk_intterval_text->set_text(std::format(L"ATK Intterval: {}", character->atk_interval));
		if (move_speed_text)
			move_speed_text->set_text(std::format(L"Move Speed: {}", character->info->nav_speed));
	}

	void enter_character_panel(cCharacterPtr _character)
	{
		reset();
		type = PanelCharacter;
		character = _character;

		if (avatar_image)
			avatar_image->set_image_name(character->info->icon_name);
		if (name_text)
		{
			cvec4 col = cvec4(255);
			if (character->faction == player1.faction)
				col = cvec4(100, 255, 100, 255);
			else if (character->faction == player2.faction)
				col = cvec4(255, 100, 100, 255);
			name_text->set_col(col);
			name_text->set_text(s2w(character->info->name));
		}
		if (hp_bar)
			hp_bar->set_enable(true);
		if (mp_bar)
			mp_bar->set_enable(true);

		if (character_stats_panel)
			character_stats_panel->set_enable(true);
	}

	void show_town_basics()
	{
		if (name_text)
		{
			cvec4 col = cvec4(255);
			if (town->player == &player1)
				col = cvec4(100, 255, 100, 255);
			else if (town->player == &player2)
				col = cvec4(255, 100, 100, 255);
			name_text->set_col(col);
			name_text->set_text(s2w(town->info->name));
		}
		if (hp_bar)
			hp_bar->set_enable(true);

		if (resources_production_panel)
		{
			if (town->player->faction == player1.faction)
				resources_production_panel->set_enable(true);
		}
	}

	void update_town_production()
	{
		if (blood_production_text)
			blood_production_text->set_text(wstr(town->get_blood_production()));
		if (bones_production_text)
			bones_production_text->set_text(wstr(town->get_bones_production()));
		if (soul_sand_production_text)
			soul_sand_production_text->set_text(wstr(town->get_soul_sand_production()));
	}

	void update_town_construction()
	{
		if (construction_list)
		{
			auto e_list = construction_list->entity;
			e_list->set_enable(true);

			static uint constructions_updated_frame = 0;
			if (constructions_updated_frame <= town->constructions_changed_frame)
			{
				constructions_updated_frame = town->constructions_changed_frame;

				construction_list->set_count(town->constructions.size());
				for (auto i = 0; i < town->constructions.size(); i++)
				{
					auto c = e_list->children[i].get();
					auto& construction = town->constructions[i];
					if (auto image = c->find_child("icon")->get_component<cImage>(); image)
						image->set_image_name(construction.building_info->icon_name);
					if (auto cancel_button = c->find_child("cancel"); cancel_button)
					{
						if (auto receiver = cancel_button->get_component<cReceiver>(); receiver)
						{
							receiver->event_listeners.clear();
							auto action = construction.action;
							receiver->event_listeners.add([this, cancel_button, action](uint type, const vec2&) {
								switch (type)
								{
								case "mouse_enter"_h:
									show_tooltip(cancel_button->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Cancel");
									break;
								case "click"_h:
									town->remove_construction(action);
									break;
								}
							});
						}
					}
				}
			}
			for (auto i = 0; i < town->constructions.size(); i++)
			{
				auto c = e_list->children[i].get();
				auto& construction = town->constructions[i];
				c->find_child_recursively("bar")->get_component<cElement>()->set_scl(vec2(1.f - construction.timer / construction.duration, 1.f));
				c->find_child_recursively("text")->get_component<cText>()->set_text(std::format(L"{}:{}", int(construction.timer / 60.f), int(construction.timer)));
			}
		}
	}

	void enter_town_panel(TownInstance* _town)
	{
		reset();
		type = PanelTownMain;
		town = _town;
		building_index = -1;

		show_town_basics();

		if (action_list)
		{
			auto last_row = get_last_row_idx(action_list);

			if (town->player == &player1)
			{
				for (auto i = 0; i < town->buildings.size(); i++)
				{
					auto c = action_list->entity->children[i].get();
					auto& building = town->buildings[i];
					if (auto image = c->get_component<cImage>(); image)
						image->set_image_name(building.info->icon_name);
					if (auto receiver = c->get_component<cReceiver>(); receiver)
					{
						receiver->event_listeners.add([this, c, i](uint type, const vec2&) {
							switch (type)
							{
							case "mouse_enter"_h:
							{
								auto& building = town->buildings[i];
								std::wstring text;
								text = s2w(building.info->name) + L'\n';
								text += s2w(building.info->description);
								show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), text);
							}
								break;
							case "click"_h:
								enter_town_building_panel(i);
								break;
							}
						});
					}
					if (auto number = c->find_child("number"); number)
					{
						number->set_enable(true);
						number->get_component<cText>()->set_text(wstr(building.number));
					}
				}

				auto c = action_list->entity->children[last_row].get();
				if (auto image = c->get_component<cImage>(); image)
					image->set_image_name(Path::get(L"assets\\extra\\icons\\build.png"));
				if (auto receiver = c->get_component<cReceiver>(); receiver)
				{
					receiver->event_listeners.add([this, c](uint type, const vec2&) {
						switch (type)
						{
						case "mouse_enter"_h:
							show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Build");
							break;
						case "click"_h:
							enter_town_construction_panel();
							break;
						}
					});
				}
			}
			else if (town->player->faction != player1.faction)
			{
				{
					auto c = action_list->entity->children[0].get();
					if (auto image = c->get_component<cImage>(); image)
						image->set_image_name(Path::get(L"assets\\extra\\icons\\attack.png"));
					if (auto receiver = c->get_component<cReceiver>(); receiver)
					{
						receiver->event_listeners.add([this, c](uint type, const vec2&) {
							switch (type)
							{
							case "mouse_enter"_h:
								show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Attack This Town");
								break;
							case "click"_h:
								enter_town_attack_panel();
								break;
							}
						});
					}
				}
				{
					auto c = action_list->entity->children[1].get();
					if (auto image = c->get_component<cImage>(); image)
						image->set_image_name(Path::get(L"assets\\extra\\icons\\cancel.png"));
					if (auto receiver = c->get_component<cReceiver>(); receiver)
					{
						receiver->event_listeners.add([this, c](uint type, const vec2&) {
							switch (type)
							{
							case "mouse_enter"_h:
								show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Cancel Actions On This Town");
								break;
							case "click"_h:
								player1.town.remove_attack_target(town->e->get_component<cNode>());
								break;
							}
						});
					}
				}
			}
		}
	}

	void enter_town_construction_panel()
	{
		auto _town = town;
		reset();
		type = PanelTownConstrucion;
		town = _town;

		show_town_basics();

		if (action_list)
		{
			auto& actions = town->info->construction_actions;
			for (auto i = 0; i < actions.size(); i++)
			{
				auto c = action_list->entity->children[i].get();
				auto construction = &actions[i];
				if (auto building_info = building_infos.find(construction->name); building_info)
				{
					if (auto image = c->get_component<cImage>(); image)
						image->set_image_name(building_info->icon_name);
					if (auto receiver = c->get_component<cReceiver>(); receiver)
					{
						receiver->event_listeners.add([this, c, construction](uint type, const vec2& v) {
							switch (type)
							{
							case "mouse_enter"_h:
							{
								if (auto building_info = building_infos.find(construction->name); building_info)
								{
									std::wstring text;
									text = s2w(building_info->name) + L'\n';
									text += L"Left - Construct";
									show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), text,
										construction->cost_blood, construction->cost_bones, construction->cost_soul_sand);
								}
							}
								break;
							case "click"_h:
								town->add_construction(construction);
								break;
							}
						});
					}
				}
			}

			auto last_row = get_last_row_idx(action_list);
			auto c = action_list->entity->children[last_row].get();
			if (auto image = c->get_component<cImage>(); image)
				image->set_image_name(Path::get(L"assets\\extra\\icons\\back.png"));
			if (auto receiver = c->get_component<cReceiver>(); receiver)
			{
				receiver->event_listeners.add([this, c](uint type, const vec2&) {
					switch (type)
					{
					case "mouse_enter"_h:
						show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Back To Main Panel");
						break;
					case "click"_h:
						enter_town_panel(town);
						break;
					}
				});
			}
		}
	}

	void enter_town_building_panel(int index)
	{
		auto _town = town;
		reset();
		type = PanelTownBuilding;
		town = _town;
		building_index = index;

		show_town_basics();

		if (action_list)
		{
			auto& building = town->buildings[index];
			if (!building.info->training_actions.empty())
			{
				for (auto i = 0; i < building.info->training_actions.size(); i++)
				{
					auto c = action_list->entity->children[i].get();
					auto& training = building.info->training_actions[i];
					if (auto unit_info = character_infos.find(training.name); unit_info)
					{
						if (auto image = c->get_component<cImage>(); image)
							image->set_image_name(unit_info->icon_name);
						if (auto receiver = c->get_component<cReceiver>(); receiver)
						{
							auto action = &building.info->training_actions[i];
							receiver->event_listeners.add([this, c, action](uint type, const vec2& v) {
								switch (type)
								{
								case "mouse_enter"_h:
								{;
									if (auto unit_info = character_infos.find(action->name); unit_info)
									{
										std::wstring text;
										text = s2w(unit_info->name) + L'\n';
										text += std::format(L"ATK: {} ({})\n", unit_info->atk, unit_info->atk_type == PhysicalDamage ? L"Physical" : L"Magical");
										text += std::format(L"ATK Distance: {}\n", unit_info->atk_distance);
										text += std::format(L"ATK Intterval: {}\n", unit_info->atk_interval);
										text += std::format(L"Move Speed: {}\n", unit_info->nav_speed);
										text += L"\nLeft - Train One\n"
											L"Right - Train Infinite";
										show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), text,
											action->cost_blood, action->cost_bones, action->cost_soul_sand);
									}
								}
									break;
								case "click"_h:
								{
									auto& building = town->buildings[building_index];
									building.add_training(action, 1);
								}
									break;
								case "mouse_up"_h:
									if (v.x == 1.f)
									{
										auto& building = town->buildings[building_index];
										building.add_training(action, -1);
									}
									break;
								}
							});
						}
					}
				}
			}

			auto last_row = get_last_row_idx(action_list);
			auto c = action_list->entity->children[last_row].get();
			if (auto image = c->get_component<cImage>(); image)
				image->set_image_name(Path::get(L"assets\\extra\\icons\\back.png"));
			if (auto receiver = c->get_component<cReceiver>(); receiver)
			{
				receiver->event_listeners.add([this, c](uint type, const vec2&) {
					switch (type)
					{
					case "mouse_enter"_h:
						show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Back To Main Panel");
						break;
					case "click"_h:
						enter_town_panel(town);
						break;
					}
				});
			}
		}
	}

	void enter_town_attack_panel()
	{
		auto _town = town;
		reset();
		type = PanelTownAttack;
		town = _town;

		show_town_basics();

		if (action_list)
		{
			std::vector<std::pair<const CharacterInfo*, uint>> available_units;
			for (auto c : player1.town.troop)
			{
				if (auto ai = c->entity->get_component<cAI>(); ai)
				{
					if (!ai->target_node)
					{
						auto info = c->info;
						auto it = std::find_if(available_units.begin(), available_units.end(), [info](const auto& p) {
							return p.first == info;
						});
						if (it == available_units.end())
							available_units.emplace_back(info, 1);
						else
							it->second++;
					}
				}
			}
			for (auto i = 0; i < available_units.size(); i++)
			{
				auto c = action_list->entity->children[i].get();
				auto& unit = available_units[i];
				auto info = unit.first;
				auto max_count = unit.second;
				if (auto image = c->get_component<cImage>(); image)
					image->set_image_name(unit.first->icon_name);
				if (auto number = c->find_child("number"); number)
				{
					number->set_enable(true);
					number->get_component<cText>()->set_text(L"0");
				}
				if (auto receiver = c->get_component<cReceiver>(); receiver)
				{
					receiver->event_listeners.add([this, c, info, max_count](uint type, const vec2& v) {
						switch (type)
						{
						case "mouse_enter"_h:
						{
							std::wstring text;
							text += s2w(info->name) + L'\n';
							text += std::format(L"Available: {}\n", max_count);
							text += L"\nLeft - Increase\n"
								L"Right - Decrease";
							show_tooltip(c->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), text);
						}
							break;
						case "click"_h:
						{
							if (auto number = c->find_child("number"); number)
							{
								auto text = number->get_component<cText>();
								auto count = s2t<uint>(text->text);
								if (count < max_count)
								{
									count++;
									text->set_text(wstr(count));
								}
							}
						}
							break;
						case "mouse_up"_h:
							if (v.x == 1.f)
							{
								if (auto number = c->find_child("number"); number)
								{
									auto text = number->get_component<cText>();
									auto count = s2t<uint>(text->text);
									if (count > 0)
									{
										count--;
										text->set_text(wstr(count));
									}
								}
							}
							break;
						}
					});
				}
			}

			auto last_row = get_last_row_idx(action_list);
			auto c1 = action_list->entity->children[last_row].get();
			if (auto image = c1->get_component<cImage>(); image)
				image->set_image_name(Path::get(L"assets\\extra\\icons\\back.png"));
			if (auto receiver = c1->get_component<cReceiver>(); receiver)
			{
				receiver->event_listeners.add([this, c1](uint type, const vec2&) {
					switch (type)
					{
					case "mouse_enter"_h:
						show_tooltip(c1->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Back To Main Panel");
						break;
					case "click"_h:
						enter_town_panel(town);
						break;
					}
				});
			}
			auto c2 = action_list->entity->children[last_row + 1].get();
			if (auto image = c2->get_component<cImage>(); image)
				image->set_image_name(Path::get(L"assets\\extra\\icons\\attack.png"));
			if (auto receiver = c2->get_component<cReceiver>(); receiver)
			{
				receiver->event_listeners.add([this, c2, available_units](uint type, const vec2&) {
					switch (type)
					{
					case "mouse_enter"_h:
						show_tooltip(c2->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Comfirm");
						break;
					case "click"_h:
					{
						auto formation = available_units;
						for (auto i = 0; i < formation.size(); i++)
							formation[i].second = s2t<uint>(action_list->entity->children[i]->find_child("number")->get_component<cText>()->text);
						player1.town.send_troop(town->e->get_component<cNode>(), formation);
					}
						break;
					}
				});
			}
		}
	}

	void update()
	{
		if (selected_target_changed && selected_target)
		{
			if (auto character = selected_target.entity->get_component<cCharacter>(); character)
			{
				enter_character_panel(character);
			}
			else if (auto town = selected_target.entity->get_component<cTown>(); town)
			{
				Player* player = nullptr;
				if (selected_target.entity->name.starts_with("player1"))
					player = &player1;
				else if (selected_target.entity->name.starts_with("player2"))
					player = &player2;
				enter_town_panel(&player->town);
			}
		}
		if (!selected_target)
			reset();

		switch (type)
		{
		case PanelCharacter:
			update_character();
			break;
		case PanelTownMain:
			if (hp_bar)
			{
				hp_bar->find_child("bar")->get_component<cElement>()->set_scl(vec2((float)town->hp / (float)town->hp_max, 1.f));
				hp_bar->find_child("text")->get_component<cText>()->set_text(wstr(town->hp) + L"/" + wstr(town->hp_max));
			}

			if (town->player == &player1)
			{
				if (attack_list)
				{
					auto e_list = construction_list->entity;
					e_list->set_enable(true);

					static uint attacks_updated_frame = 0;
					if (attacks_updated_frame <= town->attacks_changed_frame)
					{
						attacks_updated_frame = town->attacks_changed_frame;

						attack_list->set_count(town->attack_list.size());
						for (auto i = 0; i < town->attack_list.size(); i++)
						{
							auto c = e_list->children[i].get();
							auto target = town->attack_list[i];
							if (auto cancel_button = c->find_child("cancel"); cancel_button)
							{
								if (auto receiver = cancel_button->get_component<cReceiver>(); receiver)
								{
									receiver->event_listeners.clear();
									receiver->event_listeners.add([this, cancel_button, target](uint type, const vec2&) {
										switch (type)
										{
										case "mouse_enter"_h:
											show_tooltip(cancel_button->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Cancel");
											break;
										case "click"_h:
											town->remove_attack_target(target);
											break;
										}
									});
								}
							}
						}
					}
				}
			}
			if (town->player->faction == player1.faction)
				update_town_production();
			if (town->player == &player1)
				update_town_construction();
			break;
		case PanelTownConstrucion:
			if (hp_bar)
			{
				hp_bar->find_child("bar")->get_component<cElement>()->set_scl(vec2((float)town->hp / (float)town->hp_max, 1.f));
				hp_bar->find_child("text")->get_component<cText>()->set_text(wstr(town->hp) + L"/" + wstr(town->hp_max));
			}

			if (town->player->faction == player1.faction)
				update_town_production();
			if (town->player == &player1)
				update_town_construction();
			break;
		case PanelTownBuilding:
		{
			if (hp_bar)
			{
				hp_bar->find_child("bar")->get_component<cElement>()->set_scl(vec2((float)town->hp / (float)town->hp_max, 1.f));
				hp_bar->find_child("text")->get_component<cText>()->set_text(wstr(town->hp) + L"/" + wstr(town->hp_max));
			}

			if (town->player->faction == player1.faction)
				update_town_production();

			auto& building = town->buildings[building_index];
			if (training_list)
			{
				auto e_list = training_list->entity;
				e_list->set_enable(true);

				static uint training_updated_frame = 0;
				if (training_updated_frame <= building.trainings_changed_frame)
				{
					training_updated_frame = building.trainings_changed_frame;

					training_list->set_count(building.trainings.size());
					for (auto i = 0; i < building.trainings.size(); i++)
					{
						auto c = e_list->children[i].get();
						auto& training = building.trainings[i];
						if (auto image = c->find_child("icon")->get_component<cImage>(); image)
							image->set_image_name(training.unit_info->icon_name);
						if (auto split_button = c->find_child("split"); split_button)
						{
							if (auto receiver = split_button->get_component<cReceiver>(); receiver)
							{
								receiver->event_listeners.clear();
								receiver->event_listeners.add([this, split_button, i](uint type, const vec2&) {
									switch (type)
									{
									case "mouse_enter"_h:
										show_tooltip(split_button->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Split This Training Into Two");
										break;
									case "click"_h:
									{
										auto& building = town->buildings[building_index];
										auto& training = building.trainings[i];
										if (training.number > 1)
										{
											auto n = (int)floor(training.number * 0.5f);
											training.number -= n;
											building.add_training(training.action, n, true);
										}
										else if (training.number == -1)
											building.add_training(training.action, -1, true);
									}
										break;
									}
								});
							}
						}
						if (auto cancel_button = c->find_child("cancel"); cancel_button)
						{
							if (auto receiver = cancel_button->get_component<cReceiver>(); receiver)
							{
								receiver->event_listeners.clear();
								receiver->event_listeners.add([this, cancel_button, i](uint type, const vec2&) {
									switch (type)
									{
									case "mouse_enter"_h:
										show_tooltip(cancel_button->get_component<cElement>()->global_pos0() + vec2(0.f, -8.f), L"Cancel");
										break;
									case "click"_h:
									{
										auto& building = town->buildings[building_index];
										building.remove_training(i);
									}
										break;
									}
								});
							}
						}
					}
				}
				for (auto i = 0; i < building.trainings.size(); i++)
				{
					auto c = e_list->children[i].get();
					auto& training = building.trainings[i];
					c->find_child_recursively("number")->get_component<cText>()->set_text(wstr(training.number));
					c->find_child_recursively("bar")->get_component<cElement>()->set_scl(vec2(1.f - training.timer / training.duration, 1.f));
					c->find_child_recursively("text")->get_component<cText>()->set_text(std::format(L"{}:{}", int(training.timer / 60.f), int(training.timer)));
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
			e->get_component<cText>()->set_text(wstr(player1.blood));
		}
		ui_resource_bar->find_child("bones_icon")->set_enable(true);
		if (auto e = ui_resource_bar->find_child("bones_text"); e)
		{
			e->set_enable(true);
			e->get_component<cText>()->set_text(wstr(player1.bones));
		}
		ui_resource_bar->find_child("soul_sand_icon")->set_enable(true);
		if (auto e = ui_resource_bar->find_child("soul_sand_text"); e)
		{
			e->set_enable(true);
			e->get_component<cText>()->set_text(wstr(player1.soul_sand));
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

	if (selected_target)
	{
		if (auto character = selected_target.entity->get_component<cCharacter>(); character)
		{
			auto r = character->get_radius() + 0.05f;
			auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r));
			std::vector<vec3> pts(circle_draw.pts.size() * 2);
			auto center = character->node->pos;
			center.y += 0.1f;
			for (auto i = 0; i < circle_draw.pts.size(); i++)
			{
				pts[i * 2 + 0] = center + vec3(r * circle_draw.get_pt(i), 0.f).xzy();
				pts[i * 2 + 1] = center + vec3(r * circle_draw.get_pt(i + 1), 0.f).xzy();
			}
			sRenderer::instance()->draw_primitives("LineList"_h, pts.data(), pts.size(), cvec4(85, 131, 79, 255), true);
		}
	}

	if (main_camera.camera)
	{
		auto camera_x = main_camera.node->x_axis();
		for (auto character : find_characters_within_camera((FactionFlags)0xffffffff))
		{
			auto radius = character->get_radius() * 1.2f + 0.1f;
			auto height = character->get_height();
			auto pos = character->node->pos;
			auto p0 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.2f, 0.f) - camera_x * radius);
			if (p0.x < 0.f)
				continue;
			auto p1 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.2f, 0.f) + camera_x * radius);
			if (p1.x < 0.f)
				continue;
			auto p2 = main_camera.camera->world_to_screen(pos + vec3(0.f, height, 0.f) + camera_x * radius);
			if (p2.x < 0.f)
				continue;
			auto w = p1.x - p0.x; auto h = p2.y - p0.y;
			if (w > 0.f && h > 0.f)
				canvas->add_rect_filled(p0, p0 + vec2((float)character->hp / (float)character->hp_max * w, h), cvec4(80, 160, 85, 255));
		}
		for (auto town : towns)
		{
			const auto radius = 5.f;
			const auto height = 5.f;
			auto pos = town->node->global_pos();
			auto p0 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.5f, 0.f) - camera_x * radius);
			if (p0.x < 0.f)
				continue;
			auto p1 = main_camera.camera->world_to_screen(pos + vec3(0.f, height + 0.5f, 0.f) + camera_x * radius);
			if (p1.x < 0.f)
				continue;
			auto p2 = main_camera.camera->world_to_screen(pos + vec3(0.f, height, 0.f) + camera_x * radius);
			if (p2.x < 0.f)
				continue;
			auto w = p1.x - p0.x; auto h = p2.y - p0.y;
			if (w > 0.f && h > 0.f)
			{
				canvas->add_rect_filled(p0, p0 + vec2((float)town->ins->hp / (float)town->ins->hp_max * w, h), cvec4(80, 160, 85, 255));
				canvas->add_text(nullptr, 24, p0 + vec2(0.f, -24.f), wstr(town->ins->hp) + L'/' + wstr(town->ins->hp_max), cvec4(255), 0.5f, 0.2f);
			}
		}

		if (hovering_character)
		{
			std::vector<CommonDraw> ds;
			for (auto& c : hovering_character->entity->children)
			{
				for (auto mesh : c->get_components<cMesh>(1))
				{
					if (!AABB_frustum_check(main_camera.camera->frustum, mesh->node->bounds))
						continue;
					if (mesh->instance_id != -1 && mesh->mesh_res_id != -1 && mesh->material_res_id != -1)
						ds.emplace_back("mesh"_h, mesh->mesh_res_id, mesh->instance_id);
				}
			}
			sRenderer::instance()->draw_outlines(ds, hovering_character->faction == player1.faction ?
				cvec4(64, 128, 64, 255) : cvec4(128, 64, 64, 255), 4, "BOX"_h);
		}
		if (hovering_chest)
		{
			for (auto& c : hovering_chest->entity->get_all_children())
			{
				if (auto node = c->get_component<cNode>(); !node || !AABB_frustum_check(main_camera.camera->frustum, node->bounds))
					continue;
				if (auto mesh = c->get_component<cMesh>(); mesh && mesh->instance_id != -1 && mesh->mesh_res_id != -1)
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
		ui_tooltip->get_component<cElement>()->set_pos(pos);
		ui_tooltip->find_child("text")->get_component<cText>()->set_text(text);
		ui_tooltip->find_child("resources")->set_enable(false);
	}
}

void show_tooltip(const vec2& pos, const std::wstring& text, uint blood, uint bones, uint soul_sand)
{
	if (ui_tooltip)
	{
		ui_tooltip->set_enable(true);
		ui_tooltip->get_component<cElement>()->set_pos(pos);
		ui_tooltip->find_child("text")->get_component<cText>()->set_text(text);
		auto e_resources = ui_tooltip->find_child("resources");
		e_resources->set_enable(true);
		if (blood > 0)
		{
			e_resources->find_child("blood_icon")->set_enable(true);
			if (auto e = e_resources->find_child("blood_text"); e)
			{
				e->set_enable(true);
				e->get_component<cText>()->set_text(wstr(blood));
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
				e->get_component<cText>()->set_text(wstr(bones));
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
				e->get_component<cText>()->set_text(wstr(soul_sand));
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

static std::map<graphics::ImagePtr, graphics::ImagePtr> gray_icons;
graphics::ImagePtr get_gray_icon(graphics::ImagePtr icon)
{
	auto it = gray_icons.find(icon);
	if (it != gray_icons.end())
		return it->second;
	auto img = graphics::Image::create(icon->format, icon->extent, graphics::ImageUsageAttachment | graphics::ImageUsageSampled);
	{
		graphics::InstanceCommandBuffer cb;
		cb->image_barrier(img, {}, graphics::ImageLayoutAttachment);
		cb->set_viewport_and_scissor(Rect(vec2(0.f), icon->extent.xy()));
		cb->begin_renderpass(nullptr, img->get_shader_write_dst());
		auto pl = graphics::GraphicsPipeline::get(L"flame\\shaders\\grayscale.pipeline", {});
		cb->bind_pipeline(pl);
		cb->bind_descriptor_set(0, icon->get_shader_read_src());
		cb->draw(3, 1, 0, 0);
		cb->end_renderpass();
		cb->image_barrier(img, {}, graphics::ImageLayoutShaderReadOnly);
		cb.excute();
	}
	gray_icons[icon] = img;
	return img;
}
