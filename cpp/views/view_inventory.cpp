#include "view_inventory.h"
#include "../character.h"
#include "../item.h"

#include <flame/universe/components/node.h>

ViewInventory view_inventory;

ViewInventory::ViewInventory() :
	GuiView("Inventory")
{
}

void ViewInventory::on_draw()
{
	if (main_player.character)
	{
		auto dl = ImGui::GetWindowDrawList();
		const auto icon_size = 48.f;
		for (auto i = 0; i < main_player.character->inventory.size(); i++)
		{
			if (i % 10 != 0) ImGui::SameLine();
			auto pressed = ImGui::InvisibleButton(("inventory" + str(i)).c_str(), ImVec2(icon_size, icon_size));
			auto p0 = (vec2)ImGui::GetItemRectMin();
			auto p1 = (vec2)ImGui::GetItemRectMax();
			dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
			auto ins = main_player.character->inventory[i].get();
			if (ins)
			{
				auto& item = Item::get(ins->id);
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(item.name.c_str());
					ImGui::EndTooltip();
				}
				dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("item", &i, sizeof(int));
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginPopupContextItem())
				{
					if (item.type == ItemEquipment)
					{
						if (ImGui::Selectable("Equip"))
						{
							int id = ins->id;
							std::swap(id, main_player.character->equipments[item.equip_part]);
							main_player.character->inventory[i].reset(nullptr);
							if (id != -1)
								main_player.character->gain_item(id, 1);
							main_player.character->stats_dirty = true;
						}
					}
					if (ImGui::Selectable("Drop"))
					{
						add_chest(main_player.character->node->pos + vec3(linearRand(-0.2f, 0.2f), 0.f, linearRand(-0.2f, 0.2f)), ins->id, ins->num);
						main_player.character->inventory[i].reset(nullptr);
					}
					ImGui::EndPopup();
				}
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (auto payload = ImGui::AcceptDragDropPayload("item"); payload)
				{
					auto j = *(int*)payload->Data;
					if (i != j)
						std::swap(main_player.character->inventory[i], main_player.character->inventory[j]);
				}
				ImGui::EndDragDropTarget();
			}
		}
	}
}
