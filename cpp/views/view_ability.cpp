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
		for (auto i = 0; i < main_player.character->abilities.size(); i++)
		{
			if (i % 6 != 0) ImGui::SameLine();
			auto pressed = ImGui::InvisibleButton(("ability" + str(i)).c_str(), ImVec2(icon_size, icon_size));
			auto p0 = (vec2)ImGui::GetItemRectMin();
			auto p1 = (vec2)ImGui::GetItemRectMax();
			auto ins = main_player.character->abilities[i].get();
			if (ins)
			{
				auto& ability = Ability::get(ins->id);
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(ability.name.c_str());
					if (ability.show)
					{
						ImGui::Text("LV: %d", ins->lv);
						ability.show(ins);
						ins->lv++;
						ImGui::Text("LV: %d", ins->lv);
						ability.show(ins);
						ins->lv--;
					}
					ImGui::EndTooltip();
				}
				dl->AddImage(ins->lv == 0 ? get_gray_icon(ability.icon_image) : ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());
				dl->AddRectFilled(p1 - vec2(8, 15), p1, ImColor(0.f, 0.f, 0.f, 0.5f));
				dl->AddText(p1 - vec2(8, 15), ImColor(1.f, 1.f, 1.f), str(ins->lv).c_str());

				if (ins->lv > 0)
				{
					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("ability", &i, sizeof(int));
						ImGui::EndDragDropSource();
					}
				}

				if (pressed && main_player.character->ability_points > 0)
				{
					ins->lv++;
					main_player.character->ability_points--;
					main_player.character->stats_dirty = true;

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
}
