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
							spent_points += ins->lv;

							auto& ability = Ability::get(ins->id);
							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::TextUnformatted(ability.name.c_str());
								ImGui::TextUnformatted(ability.description.c_str());
								ImGui::EndTooltip();
							}
							dl->AddImage(ins->lv == 0 ? get_gray_icon(ability.icon_image) : ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());
							dl->AddText(p1 - vec2(8, 15), ImColor(1.f, 1.f, 1.f), str(ins->lv).c_str());

							auto can_level_up = false;
							if (ins->lv < ability.max_lv && main_player.character->ability_points > 0)
							{
								if (spent_points >= layer_idx * 5)
									can_level_up = true;
							}

							if (can_level_up)
								dl->AddRect(p0 - vec2(1.f), p1 + vec2(1.f), ImColor(0.f, 1.f, 0.f, 1.f));
							else if (ins->lv == ability.max_lv)
								dl->AddRect(p0 - vec2(1.f), p1 + vec2(1.f), ImColor(1.f, 0.7f, 0.f, 1.f));

							if (ins->lv > 0)
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

								if (ins->lv == 1 && ability.active)
								{
									auto found = false;
									for (auto& shortcut : shortcuts)
									{
										if (shortcut->id >> 16 == 2 && (shortcut->id & 0xfff) == ins->id)
										{
											found = true;
											break;
										}
									}
									if (!found)
									{
										for (auto& shortcut : shortcuts)
										{
											if (shortcut->id < 0)
											{
												auto key = shortcut->key;
												shortcut.reset(new AbilityShortcut(ins));
												shortcut->key = key;
												break;
											}
										}
									}
								}

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
