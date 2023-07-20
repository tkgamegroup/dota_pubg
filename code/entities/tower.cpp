#include "../game.h"
#include "character.h"
#include "projectile.h"
#include "ai.h"
#include "collider.h"
#include "town.h"
#include "tower.h"

std::vector<cTowerPtr> towers;

cTower::~cTower()
{
	std::erase_if(towers, [this](const auto& i) {
		return i == this;
	});
}

void cTower::start()
{
	towers.push_back(this);

	info = tower_infos.find(info_name);

	hp_max = info->hp_max;
	hp = hp_max;

	atk = info->atk;
	atk_distance = info->atk_distance;
	atk_interval = info->atk_interval;

	if (auto collider = entity->get_component<cCircleCollider>(); collider)
	{
		collider->callbacks.add([this](cCharacterPtr character, uint type) {
			if (type == "enter"_h)
			{
				if (auto ai = character->entity->get_component<cAI>(); ai)
				{
					if (ai->target_node == entity->get_component<cNode>())
					{
						if (!player)
						{
							character->die("removed"_h);
							hp--;
							if (hp == 0)
							{
								hp = hp_max;
								auto find_character_player = [](Player* player, cCharacterPtr character)->Player* {
									if (player->town)
									{
										for (auto c : player->town->troop)
										{
											if (c == character)
												return player;
										}
									}
									return nullptr;
								};
								player = find_character_player(&player1, character);
								if (!player)
									player = find_character_player(&player2, character);
							}
						}
					}
				}
			}
		});
	}

	if (!info->atk_projectile_name.empty())
		atk_projectile = projectile_infos.find(info->atk_projectile_name);
}

void cTower::update()
{
	if (!player)
	{
		target.reset();
		return;
	}

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (attack_interval_timer > 0)
			attack_interval_timer -= delta_time;

		if (!target.comp)
		{
			if (search_timer <= 0.f)
			{
				auto enemies = find_characters_within_circle(~player->faction, node->pos, atk_distance);
				if (!enemies.empty())
					target.set(enemies[0]);
				search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
			}
		}
		else
		{
			auto p0 = node->pos;
			auto p1 = target.get<cCharacterPtr>()->node->pos;

			if (attack_interval_timer <= 0.f)
			{
				if (distance(p0, p1) < atk_distance + 3.5f)
				{
					if (atk_projectile)
					{
						auto tower = this;
						auto pj = add_projectile(atk_projectile, p0 + vec3(0.f, 6.f, 0.f), target.get<cCharacterPtr>(), 6.f);
						pj->on_end = [tower](const vec3&, cCharacterPtr target) {
							if (target)
							{
								auto damage = tower->atk;
								target->take_damage(PhysicalDamage, damage);
							}
						};
					}
				}
				else
					target.reset();
			}
		}
	}

}

struct cTowerCreate : cTower::Create
{
	cTowerPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cTower;
	}
}cTower_create;
cTower::Create& cTower::create = cTower_create;
