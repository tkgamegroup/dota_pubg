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
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\blood tornado b.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			character->state = State(character->state | StateStun);
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Flame Weapon";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma pulverize.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			character->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				character->inflict_damage(target, 10, MagicDamage);
			});
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Roar";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\fungusfungusbite2.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			character->atk += 20;
			character->mov_sp += 20;
			character->atk_sp += 100;
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Poisoned";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\fungusgue-ball.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.start = [](BuffInstance* ins, cCharacterPtr character) {
			ins->f0 = ins->timer;
		};
		buff.continuous = [](BuffInstance* ins, cCharacterPtr character) {
			if (ins->f0 - ins->timer >= 1.f)
			{
				character->take_damage(character->hp_max * 0.002f, MagicDamage);
				ins->f0 = ins->timer;
			}
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Cursed";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\blood scavanger.jpg";
		buff.icon_image = graphics::Image::get(buff.icon_name);
		buff.start = [](BuffInstance* ins, cCharacterPtr character) {
			ins->f0 = main_player.character ? (main_player.character->lv - 1) * 0.05f : 0.f;
		};
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			if (ins->f0 > 0.f)
			{
				character->hp_max *= (1.f + ins->f0);
				character->atk *= (1.f + ins->f0);
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
