#include "view_equipment.h"
#include "../character.h"

#include <flame/graphics/image.h>

ViewEquipment view_equipment;

ViewEquipment::ViewEquipment() :
	GuiView("Equipment")
{
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
		ImGui::TextUnformatted("ATK DMG");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->atk);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Physical DEF");
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->phy_def);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Magic DEF");
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

	auto dl = ImGui::GetWindowDrawList();
	auto equipment_slot = [&](EquipPart part) {
		ImGui::InvisibleButton(("equip" + str((int)part)).c_str(), ImVec2(32, 32));
		auto p0 = (vec2)ImGui::GetItemRectMin();
		auto p1 = (vec2)ImGui::GetItemRectMax();
		dl->AddRect(p0, p1, ImColor(1.f, 1.f, 0.7f));

		if (auto id = main_player.character->equipments[part]; id != -1)
		{
			auto& item = Item::get(id);
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(item.name.c_str());
				ImGui::EndTooltip();
			}
			dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::Selectable("Take Off"))
				{
					if (main_player.character->gain_item(id, 1))
					{
						main_player.character->equipments[part] = -1;
						main_player.character->stats_dirty = true;
					}
				}
				ImGui::EndPopup();
			}
		}
	};
	ImGui::SetCursorPos(offset + vec2(0.f, 20.f));
	equipment_slot(EquipHead);
	ImGui::SetCursorPos(offset + vec2(0.f, 54.f));
	equipment_slot(EquipNeck);
	ImGui::SetCursorPos(offset + vec2(0.f, 88.f));
	equipment_slot(EquipShoulder);
	ImGui::SetCursorPos(offset + vec2(0.f, 122.f));
	equipment_slot(EquipChest);
	ImGui::SetCursorPos(offset + vec2(0.f, 156.f));
	equipment_slot(EquipBack);
	ImGui::SetCursorPos(offset + vec2(134.f, 20.f));
	equipment_slot(EquipHand);
	ImGui::SetCursorPos(offset + vec2(134.f, 54.f));
	equipment_slot(EquipLeg);
	ImGui::SetCursorPos(offset + vec2(134.f, 88.f));
	equipment_slot(EquipFoot);
	ImGui::SetCursorPos(offset + vec2(134.f, 122.f));
	equipment_slot(EquipFinger0);
	ImGui::SetCursorPos(offset + vec2(134.f, 156.f));
	equipment_slot(EquipFinger1);
	ImGui::SetCursorPos(offset + vec2(50.f, 200.f));
	equipment_slot(EquipWeapon0);
	ImGui::SetCursorPos(offset + vec2(84.f, 200.f));
	equipment_slot(EquipWeapon1);
}
