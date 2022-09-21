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
		buff.passive = [](cCharacterPtr character, BuffInstance*) {
			character->state = State(character->state | StateStun);
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Flame Weapon";
		buff.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\magma pulverize.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](cCharacterPtr character, BuffInstance*) {
			character->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				character->inflict_damage(target, 10, MagicDamage);
			});
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Roar";
		buff.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\fungusfungusbite2.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](cCharacterPtr character, BuffInstance*) {
			character->atk += 20;
			character->mov_sp += 20;
			character->atk_sp += 100;
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Poisoned";
		buff.icon_name = L"assets\\icons\\abilities\\old Ancient Beast icons\\fungusgue-ball.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.start = [](cCharacterPtr character, BuffInstance* ins) {
			ins->f0 = ins->timer;
		};
		buff.continuous = [](cCharacterPtr character, BuffInstance* ins) {
			if (ins->f0 - ins->timer >= 1.f)
			{
				character->take_damage(character->hp_max * 0.002f, MagicDamage);
				ins->f0 = ins->timer;
			}
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
