#include "view_ability.h"
#include "../ability.h"
#include "../character.h"

#include <flame/graphics/image.h>
#include <flame/graphics/extension.h>

ViewAbility view_ability;

ViewAbility::ViewAbility() :
	GuiView("Ability")
{
}

bool ViewAbility::on_begin()
{
	bool open = true;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(40, 40, 40));
	ImGui::Begin(name.c_str(), modal ? nullptr : &open, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();
	return !open;
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

void ViewAbility::on_draw()
{
	if (main_player.character)
	{
		auto dl = ImGui::GetWindowDrawList();
		const auto icon_size = 48.f;

		ImGui::Text("Points: %d", main_player.character->ability_points);
		ImGui::BeginTabBar("##talents");
		for (auto& tid : main_player.character->talents)
		{
			auto& talent = Talent::get(tid);
			if (ImGui::BeginTabItem(talent.name.c_str()))
			{
				auto find_ablility = [](uint id) {
					for (auto i = 0; i < main_player.character->abilities.size(); i++)
					{
						if (main_player.character->abilities[i]->id == id)
							return i;
					}
					return -1;
				};

				auto spent_points = 0;
				for (auto layer_idx = 0; layer_idx < talent.ablilities_list.size(); layer_idx++)
				{
					auto& layer = talent.ablilities_list[layer_idx];
					auto first = true;
					for (auto id : layer)
					{
						if (!first) ImGui::SameLine();
						first = false;

						auto i = find_ablility(id);
						auto pressed = ImGui::InvisibleButton(("ability" + str(i)).c_str(), ImVec2(icon_size, icon_size));
						auto p0 = (vec2)ImGui::GetItemRectMin();
						auto p1 = (vec2)ImGui::GetItemRectMax();
						auto ins = main_player.character->abilities[i].get();
						if (ins)
						{
							auto& ability = Ability::get(ins->id);
							auto lv = ins->lv;
							spent_points += lv;

							auto can_level_up = lv < ability.max_lv && main_player.character->ability_points > 0 && spent_points >= layer_idx * 5;

							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::TextUnformatted(ability.name.c_str());
								ImGui::PushStyleColor(ImGuiCol_Text, (uint)ImColor(0.8f, 0.8f, 0.8f, 1.f));
								if (!ability.active)
									ImGui::TextUnformatted("Passive");
								else
								{
									switch (ability.target_type)
									{
									case TargetNull:
										break;
									case TargetEnemy:
										ImGui::TextUnformatted("Target: Enemy");
										break;
									case TargetFriendly:
										ImGui::TextUnformatted("Target: Friendly");
										break;
									case TargetLocation:
										ImGui::TextUnformatted("Target: Location");
										break;
									}
									ImGui::Text("MP: %d", ability.get_mp(lv));
									if (can_level_up && ability.mp.size() > 1)
									{
										ImGui::SameLine();
										ImGui::Text("-> %d", ability.get_mp(lv + 1));
									}
									ImGui::Text("CD: %.1f", ability.get_cd(lv));
									if (can_level_up && ability.cd.size() > 1)
									{
										ImGui::SameLine();
										ImGui::Text("-> %.1f", ability.get_cd(lv + 1));
									}
									if (ability.target_type != TargetNull)
									{
										ImGui::Text("Distance: %.1f", ability.get_distance(lv));
										if (can_level_up && ability.distance.size() > 1)
										{
											ImGui::SameLine();
											ImGui::Text("-> %.1f", ability.get_distance(lv + 1));
										}
									}
								}
								ImGui::TextUnformatted(ability.description.c_str());
								for (auto& p : ability.parameter_names)
								{
									auto& vec = ability.parameters.at(p.second);
									ImGui::Text("%s: %s", get_show_name(p.first).c_str(), lv == 0 ? "0" : vec.size() == 1 ? vec[0].to_str().c_str() : vec[lv - 1].to_str().c_str());
									if (can_level_up && vec.size() > 1)
									{
										ImGui::SameLine();
										ImGui::Text("-> %s", vec[lv].to_str().c_str());
									}
								}
								ImGui::PopStyleColor();
								ImGui::EndTooltip();
							}
							dl->AddImage(lv == 0 ? get_gray_icon(ability.icon_image) : ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());
							dl->AddText(p1 - vec2(8, 15), ImColor(1.f, 1.f, 1.f), str(lv).c_str());

							if (can_level_up)
								dl->AddRect(p0 - vec2(1.f), p1 + vec2(1.f), ImColor(0.f, 1.f, 0.f, 1.f));
							else if (lv == ability.max_lv)
								dl->AddRect(p0 - vec2(1.f), p1 + vec2(1.f), ImColor(1.f, 0.7f, 0.f, 1.f));

							if (lv > 0)
							{
								if (ImGui::BeginDragDropSource())
								{
									ImGui::SetDragDropPayload("ability", &i, sizeof(int));
									ImGui::EndDragDropSource();
								}
							}

							if (can_level_up && pressed && main_player.character->ability_points > 0)
							{
								ins->lv++;
								main_player.character->ability_points--;
								main_player.character->stats_dirty = true;

								for (auto& cb : main_player.character->message_listeners.list)
									cb.first(CharacterAbilityLevelUp, { .i = i }, {}, {}, {});

								if (main_player.character->ability_points == 0 && modal)
								{
									enable_game(true);

									modal = false;
									close();
								}
							}
						}
					}
				}

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
}
