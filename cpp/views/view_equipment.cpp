#include "view_equipment.h"

#include "../character.h"

ViewEquipment view_equipment;

ViewEquipment::ViewEquipment() :
	GuiView("Equipment")
{
}

void ViewEquipment::on_draw()
{
	if (ImGui::BeginTable("##t1", 2));
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Vigor");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your max HP and HP recover.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->VIG);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Mind");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your max MP and MP recover.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->MND);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Strength");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->STR);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Dexterity");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->DEX);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Intelligence");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->INT);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Luck");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->LUK);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Attack Damage");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->atk);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Physical Defense");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->phy_def);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Magic Defense");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->mag_def);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Movement Speed");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->mov_sp);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("HP Recover");
		ImGui::TableNextColumn();
		ImGui::Text("%5.1f", main_player.character->hp_rec / 10.f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("MP Recover");
		ImGui::TableNextColumn();
		ImGui::Text("%5.1f", main_player.character->mp_rec / 10.f);

		ImGui::EndTable();
	}
}
