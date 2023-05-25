#include "player.h"
#include "entities/character.h"

void Player::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->node();
		nav_agent = e->get_component_t<cNavAgent>();
		character = e->get_component_t<cCharacter>();

		character->message_listeners.add([](CharacterMessage msg, sVariant p0, sVariant p1, sVariant p2, sVariant p3) {
			//auto find_shortcut = [](Shortcut::Type type, int id) {
			//	for (auto& shortcut : shortcuts)
			//	{
			//		if (shortcut->type == type && shortcut->id == id)
			//			return true;
			//	}
			//	return false;
			//};

			//switch (msg)
			//{
			//case CharacterGainItem:
			//{
			//	auto ins = main_player.character->inventory[p2.i].get();
			//	if (Item::get(ins->id).active && !find_shortcut(Shortcut::tItem, ins->id))
			//	{
			//		for (auto& shortcut : shortcuts)
			//		{
			//			if (shortcut->type == Shortcut::tNull)
			//			{
			//				auto key = shortcut->key;
			//				shortcut.reset(new ItemShortcut(ins));
			//				shortcut->key = key;
			//				break;
			//			}
			//		}
			//	}
			//}
			//	break;
			//case CharacterGainAbility:
			//{
			//	auto ins = main_player.character->abilities[p2.i].get();
			//	if (ins->lv > 0 && Ability::get(ins->id).active && !find_shortcut(Shortcut::tAbility, ins->id))
			//	{
			//		for (auto& shortcut : shortcuts)
			//		{
			//			if (shortcut->type == Shortcut::tNull)
			//			{
			//				auto key = shortcut->key;
			//				shortcut.reset(new AbilityShortcut(ins));
			//				shortcut->key = key;
			//				break;
			//			}
			//		}
			//	}
			//}
			//	break;
			//case CharacterAbilityLevelUp:
			//{
			//	auto ins = main_player.character->abilities[p0.i].get();
			//	if (ins->lv == 1 && Ability::get(ins->id).active && !find_shortcut(Shortcut::tAbility, ins->id))
			//	{
			//		for (auto& shortcut : shortcuts)
			//		{
			//			if (shortcut->type == Shortcut::tNull)
			//			{
			//				auto key = shortcut->key;
			//				shortcut.reset(new AbilityShortcut(ins));
			//				shortcut->key = key;
			//				break;
			//			}
			//		}
			//	}
			//}
			//	break;
			//}
			});
	}
}

std::vector<BuildingInfo> Player::get_avaliable_building_infos() const
{
	auto preset = (BuildingInfosPreset*)load_preset_file(L"assets\\building_infos.preset");
	if (preset)
	{
		return preset->infos;
	}

	return {};
}

std::vector<UnitInfo> Player::get_avaliable_unit_infos() const
{
	auto preset = (UnitInfosPreset*)load_preset_file(L"assets\\unit_infos.preset");
	if (preset)
	{
		return preset->infos;
	}

	return {};
}

Player main_player;
