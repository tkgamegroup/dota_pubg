#include "view_ability.h"
#include "../ability.h"
#include "../character.h"

ViewAbility view_ability;

ViewAbility::ViewAbility() :
	GuiView("Ability")
{
}

bool ViewAbility::on_open()
{
	bool open = true;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(40, 40, 40));
	ImGui::Begin(name.c_str(), &open, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();
	return !open;
}

void ViewAbility::on_draw()
{
	if (main_player.character)
	{
		auto dl = ImGui::GetWindowDrawList();
		const auto icon_size = 48.f;
		//auto points = main_player.character->points.get();
		//ImGui::Text("Points: %d", points->ability_points);
		//for (auto i = 0; i < main_player.character->abilities.size(); i++)
		//{
		//	if (i % 6 != 0) ImGui::SameLine();
		//	auto pressed = ImGui::InvisibleButton(("ability" + str(i)).c_str(), ImVec2(icon_size, icon_size));
		//	auto p0 = (vec2)ImGui::GetItemRectMin();
		//	auto p1 = (vec2)ImGui::GetItemRectMax();
		//	auto ins = main_player.character->abilities[i].get();
		//	if (ins)
		//	{
		//		auto& ability = Ability::get(ins->id);
		//		if (ImGui::IsItemHovered())
		//		{
		//			ImGui::BeginTooltip();
		//			ImGui::TextUnformatted(ability.name.c_str());
		//			if (ability.show)
		//				ability.show();
		//			ImGui::EndTooltip();
		//		}
		//		dl->AddImage(ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());
		//		if (ins->lv == 0)
		//			dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
		//		dl->AddRectFilled(p1 - vec2(8, 15), p1, ImColor(0.f, 0.f, 0.f, 0.5f));
		//		dl->AddText(p1 - vec2(8, 15), ImColor(1.f, 1.f, 1.f), str(ins->lv).c_str());

		//		if (ins->lv > 0)
		//		{
		//			if (ImGui::BeginDragDropSource())
		//			{
		//				ImGui::SetDragDropPayload("ability", &i, sizeof(int));
		//				ImGui::EndDragDropSource();
		//			}
		//		}

		//		if (pressed && points->ability_points > 0)
		//		{
		//			ins->lv++;
		//			points->ability_points--;
		//		}
		//	}
		//}
	}
}
