#include "buff.h"
#include "character.h"

#include <flame/graphics/image.h>

std::vector<Buff> buffs;

void load_buffs()
{
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Stun";
		buff.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\blood tornado b.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](cCharacterPtr character) {
			character->state = State(character->state | StateStun);
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Flame Weapon";
		buff.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma pulverize.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](cCharacterPtr character) {
			character->atk_effs.add([](cCharacterPtr character, cCharacterPtr target) {
				character->inflict_damage(target, 10, MagicDamage);
			});
		};
	}
}

int Buff::find(const std::string& name)
{
	if (buffs.empty())
		load_buffs();
	for (auto i = 0; i < buffs.size(); i++)
	{
		if (buffs[i].name == name)
			return i;
	}
	return -1;
}

const Buff& Buff::get(uint id)
{
	if (buffs.empty())
		load_buffs();
	return buffs[id];
}
