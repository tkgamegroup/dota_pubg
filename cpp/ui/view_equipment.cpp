#include <flame/graphics/image.h>

#include "ui.h"
#include "view_equipment.h"
#include "../game.h"
#include "../entities/character.h"
#include "../entities/buff.h"

ViewEquipment view_equipment;

ViewEquipment::ViewEquipment() :
	GuiView("Equipment")
{
}

bool ViewEquipment::on_begin()
{
	bool open = true;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(40, 40, 40));
	ImGui::Begin(name.c_str(), &open, ImGuiWindowFlags_NoCollapse);
	ImGui::PopStyleColor();
	return !open;
}

void ViewEquipment::on_draw()
{
	auto offset = (vec2)ImGui::GetCursorPos();

	const auto icon_size = 48.f;
	static auto img = graphics::Image::get("assets\\icons\\Human_Silhouette.png");
	ImGui::SetCursorPos(offset + vec2(50.f, 20.f));
	ImGui::Image(img, ImVec2(64, 178));
		
	ImGui::SameLine();

	ImGui::SetCursorPos(offset + vec2(195.f, 20.f));
	if (main_player.character)
	{
		if (ImGui::BeginTable("##t1", 3));
		{

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("ATK DMG");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Attack damage.");
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn();
			ImGui::Text("%5d", main_player.character->atk);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Physical DEF");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Physical Defense.");
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn();
			ImGui::Text("%5d", main_player.character->phy_def);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Magic DEF");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Magic Defense.");
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn();
			ImGui::Text("%5d", main_player.character->mag_def);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("Move SP");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Move speed.");
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn();
			ImGui::Text("%5d", main_player.character->mov_sp);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("ATK SP");
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted("Attack speed.");
				ImGui::EndTooltip();
			}
			ImGui::TableNextColumn();
			ImGui::Text("%5d", main_player.character->atk_sp);

			ImGui::EndTable();
		}
	}
}
