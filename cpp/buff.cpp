#include "buff.h"
#include "character.h"

#include <flame/graphics/image.h>

std::vector<Buff> buffs;

void init_buffs()
{
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Stun";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\blood tornado b.jpg";
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			character->state = State(character->state | StateStun);
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Flame Weapon";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\magma pulverize.jpg";
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			character->attack_effects.add([](cCharacterPtr character, cCharacterPtr target, DamageType, uint) {
				character->inflict_damage(target, MagicDamage, 10);
			});
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Roar";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\fungusfungusbite2.jpg";
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
		buff.start = [](BuffInstance* ins, cCharacterPtr character) {
			ins->f0 = ins->timer;
		};
		buff.continuous = [](BuffInstance* ins, cCharacterPtr character) {
			if (ins->f0 - ins->timer >= 1.f)
			{
				character->take_damage(MagicDamage, character->hp_max * 0.002f);
				ins->f0 = ins->timer;
			}
		};
	}
	{
		auto& buff = buffs.emplace_back();
		buff.id = buffs.size() - 1;
		buff.name = "Cursed";
		buff.icon_name = L"assets\\icons\\old Ancient Beast icons\\blood scavanger.jpg";
		buff.start = [](BuffInstance* ins, cCharacterPtr character) {
			ins->f0 = uint(gtime / 60.f) * 0.05f;
		};
		buff.passive = [](BuffInstance* ins, cCharacterPtr character) {
			if (ins->f0 > 0.f)
			{
				character->hp_max *= (1.f + ins->f0);
				character->atk *= (1.f + ins->f0);
			}
		};
	}

	for (auto& buff : buffs)
	{
		if (!buff.icon_name.empty())
		{
			buff.icon_image = graphics::Image::get(buff.icon_name);
			if (buff.icon_image)
			{
				auto tile_size = vec2(buff.icon_image->tile_size);
				if (tile_size != vec2(0.f))
					buff.icon_uvs = vec4(vec2(buff.icon_tile_coord) / tile_size, vec2(buff.icon_tile_coord + 1U) / tile_size);
			}
		}
	}
}

int Buff::find(const std::string& name)
{
	for (auto i = 0; i < buffs.size(); i++)
	{
		if (buffs[i].name == name)
			return i;
	}
	return -1;
}

const Buff& Buff::get(uint id)
{
	return buffs[id];
}
