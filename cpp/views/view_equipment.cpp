#include "view_equipment.h"
#include "../character.h"
#include "../buff.h"

#include <flame/graphics/image.h>

ViewEquipment view_equipment;

ViewEquipment::ViewEquipment() :
	GuiView("Equipment")
{
}

bool ViewEquipment::on_open()
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

	auto points = main_player.character->points.get();
	ImGui::SetCursorPos(offset + vec2(195.f, 20.f));
	if (ImGui::BeginTable("##t1", 3));
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
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->VIG_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Mind");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your max MP and MP regeneration.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->MND);
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->MND_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Strength");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your physical attack damage and physical ability damage.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->STR);
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->STR_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Dexterity");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your attack speed and cast speed.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->DEX);
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->DEX_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Intelligence");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your magic attack damage and magic ability damage.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->INT);
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->INT_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Luck");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Increase your chance to perform a Critical hit.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5d", main_player.character->LUK);
		if (points->attribute_points > 0)
		{
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("+"))
			{
				points->LUK_PTS++;
				points->attribute_points--;
				main_player.character->stats_dirty = true;
			}
		}

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

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("HP REG");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("HP regeneration pre second.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5.1f", main_player.character->hp_reg / 10.f);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("MP REG");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("MP regeneration pre second.");
			ImGui::EndTooltip();
		}
		ImGui::TableNextColumn();
		ImGui::Text("%5.1f", main_player.character->mp_reg / 10.f);

		ImGui::EndTable();
	}

	auto dl = ImGui::GetWindowDrawList();
	auto equipment_slot = [&](EquipPart part) {
		ImGui::InvisibleButton(("equip" + str((int)part)).c_str(), ImVec2(32, 32));
		auto p0 = (vec2)ImGui::GetItemRectMin();
		auto p1 = (vec2)ImGui::GetItemRectMax();
		dl->AddRect(p0, p1, ImColor(1.f, 1.f, 0.7f));

		if (auto& ins = main_player.character->equipments[part]; ins.id != -1)
		{
			auto& item = Item::get(ins.id);
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(item.name.c_str());
				if (ins.enchant != -1)
				{
					auto& buff = Buff::get(ins.enchant);
					ImGui::Image(buff.icon_image, ImVec2(16, 16), buff.icon_uvs.xy(), buff.icon_uvs.zw());
					ImGui::SameLine();
					ImGui::TextUnformatted(buff.name.c_str());
					ImGui::SameLine();
					ImGui::Text("%d s", (int)ins.enchant_timer);
				}
				ImGui::EndTooltip();
			}
			dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::Selectable("Take Off"))
				{
					if (main_player.character->gain_item(ins.id, 1))
					{
						main_player.character->equipments[part] = { -1, -1 };
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
