#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/nav_obstacle.h>
#include <flame/universe/components/audio_source.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "../player.h"
#include "../map.h"
#include "../ui/floating_text.h"
#include "character.h"
#include "object.h"
#include "item.h"
#include "ability.h"
#include "buff.h"
#include "projectile.h"
#include "chest.h"
#include "shop.h"


std::vector<cCharacterPtr> characters;
std::unordered_map<uint, std::vector<cCharacterPtr>> factions;
std::vector<cCharacterPtr> dead_characters;
bool removing_dead_characters = false;

float cCharacter::get_radius()
{
	if (nav_agent)
		return nav_agent->radius;
	if (nav_obstacle)
		return nav_obstacle->radius;
	return 0.f;
}

float cCharacter::get_height()
{
	if (nav_agent)
		return nav_agent->height;
	if (nav_obstacle)
		return nav_obstacle->height;
	return 0.f;
}

vec3 cCharacter::get_pos(float height_factor)
{
	auto pos = node->pos;
	pos.y += get_height() * height_factor;
	return pos;
}

void cCharacter::set_faction(FactionFlags _faction)
{
	if (faction == _faction)
		return;
	std::erase_if(factions[faction], [this](const auto& i) {
		return i == this;
	});
	faction = _faction;
	factions[faction].push_back(this);

	if (nav_agent)
		nav_agent->separation_group = faction;
}

void cCharacter::set_lv(uint v)
{
	if (lv == v)
		return;
	lv = v;
	data_changed("lv"_h);
}

void cCharacter::set_exp(uint v)
{
	if (exp == v)
		return;
	exp = v;
	data_changed("exp"_h);
}

void cCharacter::set_exp_max(uint v)
{
	if (exp_max == v)
		return;
	exp_max = v;
	data_changed("exp_max"_h);
}

void cCharacter::set_hp(uint v)
{
	if (hp == v)
		return;
	hp = v;
	data_changed("hp"_h);
}

void cCharacter::set_hp_max(uint v)
{
	if (hp_max == v)
		return;
	hp_max = v;
	data_changed("hp_max"_h);
}

void cCharacter::set_mp(uint v)
{
	if (mp == v)
		return;
	mp = v;
	data_changed("mp"_h);
}

void cCharacter::set_mp_max(uint v)
{
	if (mp_max == v)
		return;
	mp_max = v;
	data_changed("mp_max"_h);
}

void cCharacter::set_atk_type(DamageType v)
{
	if (atk_type == v)
		return;
	atk_type = v;
	data_changed("atk_type"_h);
}

void cCharacter::set_atk(uint v)
{
	if (atk == v)
		return;
	atk = v;
	data_changed("atk"_h);
}

void cCharacter::set_atk_distance(float v)
{
	if (atk_distance == v)
		return;
	atk_distance = v;
	data_changed("atk_distance"_h);
}

void cCharacter::set_phy_def(uint v)
{
	if (phy_def == v)
		return;
	phy_def = v;
	data_changed("phy_def"_h);
}

void cCharacter::set_mag_def(uint v)
{
	if (mag_def == v)
		return;
	mag_def = v;
	data_changed("mag_def"_h);
}

void cCharacter::set_hp_reg(uint v)
{
	if (hp_reg == v)
		return;
	hp_reg = v;
	data_changed("hp_reg"_h);
}

void cCharacter::set_mp_reg(uint v)
{
	if (mp_reg == v)
		return;
	mp_reg = v;
	data_changed("mp_reg"_h);
}

void cCharacter::set_mov_sp(uint v)
{
	if (mov_sp == v)
		return;
	mov_sp = v;
	data_changed("mov_sp"_h);
}

void cCharacter::set_atk_sp(uint v)
{
	if (atk_sp == v)
		return;
	atk_sp = v;
	data_changed("atk_sp"_h);
}

cCharacter::~cCharacter()
{
	node->measurers.remove("character"_h);

	if (armature)

	std::erase_if(characters, [this](const auto& i) {
		return i == this;
	});
	std::erase_if(factions[faction], [this](const auto& i) {
		return i == this;
	});
	if (dead && !removing_dead_characters)
	{
		std::erase_if(dead_characters, [this](const auto& i) {
			return i == this;
		});
	}
}

void cCharacter::on_init()
{
	nav_agent = entity->get_component_t<cNavAgent>();
	if (nav_agent)
		nav_agent->separation_group = faction;
	nav_obstacle = entity->get_component_t<cNavObstacle>();
	audio_source = entity->get_component_t<cAudioSource>();

	auto e = entity;
	while (e)
	{
		if (armature = e->get_component_t<cArmature>(); armature)
			break;
		if (e->children.empty())
			break;
		e = e->children[0].get();
	}

	node->measurers.add([this](AABB& b) {
		auto radius = get_radius();
		auto height = get_height();
		if (radius > 0.f && height > 0.f)
		{
			AABB _b(vec3(-radius, 0.f, -radius), vec3(radius, height, radius));
			mat4 m(1.f);
			m = translate(mat4(1.f), node->global_pos());
			m = m * mat4(node->g_qut);
			b.expand(AABB(_b.get_points(m)));
		}
	}, "character"_h);
}

void cCharacter::start()
{
	entity->tag = entity->tag | CharacterTag;

	entity->traversal_bfs([this](EntityPtr e, int depth) {
		if (auto a = e->get_component_t<cArmature>(); a)
			armature = a;
		if ((armature) || depth > 2)
			return false; 
	});

	if (audio_source)
	{
		std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
		audio_buffer_names.emplace_back(L"assets\\level_up.wav", "level_up");
		if (!move_sound_path.empty() && std::filesystem::exists(move_sound_path))
			audio_buffer_names.emplace_back(move_sound_path, "move");
		if (!attack_precast_sound_path.empty() && std::filesystem::exists(attack_precast_sound_path))
			audio_buffer_names.emplace_back(attack_precast_sound_path, "attack_precast");
		if (!attack_hit_sound_path.empty() && std::filesystem::exists(attack_hit_sound_path))
			audio_buffer_names.emplace_back(attack_hit_sound_path, "attack_hit");
		audio_source->set_buffer_names(audio_buffer_names);
	}

	init_stats.hp_max = hp_max;
	init_stats.mp_max = mp_max;
	init_stats.atk_type = atk_type;
	init_stats.atk = atk;
	init_stats.phy_def = phy_def;
	init_stats.mag_def = mag_def;
	init_stats.hp_reg = hp_reg;
	init_stats.mp_reg = mp_reg;
	init_stats.mov_sp = mov_sp;
	init_stats.atk_sp = atk_sp;

	items_idx = entity->find_child_i("items");
	abilities_idx = entity->find_child_i("abilities");
	buffs_idx = entity->find_child_i("buffs");
	attack_effects_idx = entity->find_child_i("attack_effects");
}

void cCharacter::update()
{
	if (dead)
		return;

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (stats_dirty)
		{
			if (exp_max == 0)
				exp_max = exp_base;

			state = CharacterStateNormal;

			auto old_hp_max = hp_max;
			auto old_mp_max = mp_max;
			hp_max = init_stats.hp_max;
			mp_max = init_stats.mp_max;

			atk_type = init_stats.atk_type;
			atk = init_stats.atk;

			phy_def = init_stats.phy_def;
			mag_def = init_stats.mag_def;
			hp_reg = init_stats.hp_reg;
			mp_reg = init_stats.mp_reg;
			mov_sp = init_stats.mov_sp;
			atk_sp = init_stats.atk_sp;

			//attack_effects.clear();
			//for (auto ability : abilities)
			//{
			//	if (ability && ability->lv > 0)
			//	{
			//		if (!ability->passive.cmds.empty())
			//			CommandListExecuteThread(ability->passive, this, nullptr, vec3(0.f), ability->parameters, ability->lv).execute();
			//	}
			//}
			//for (auto buff : buffs)
			//{
			//	if (!buff->passive.cmds.empty())
			//		CommandListExecuteThread(buff->passive, this, nullptr, vec3(0.f), buff->parameters, buff->lv).execute();
			//}

			if (hp_max != old_hp_max)
			{
				set_hp(hp * (float)hp_max / old_hp_max);
				data_changed("hp_max"_h);
			}
			if (mp_max != old_hp_max)
			{
				set_mp(mp * (float)mp_max / old_mp_max);
				data_changed("mp_max"_h);
			}

			stats_dirty = false;
		}

		if (regeneration_timer > 0)
			regeneration_timer -= delta_time;
		else
		{
			set_hp(min(hp + hp_reg, hp_max));
			set_mp(min(mp + mp_reg, mp_max));
			regeneration_timer = 1.f;
		}

		if (search_timer > 0)
			search_timer -= delta_time;
		if (attack_interval_timer > 0)
			attack_interval_timer -= delta_time;

		//for (auto& ins : abilities)
		//{
		//	if (ins && ins->cd_timer > 0.f)
		//		ins->cd_timer -= delta_time;
		//}
		//for (auto it = buffs.begin(); it != buffs.end();)
		//{
		//	auto& ins = *it;
		//	auto& buff = Buff::get(ins->id);

		//	if (ins->t0 - ins->timer > ins->duration)
		//	{
		//		if (!buff.continuous.cmds.empty())
		//			CommandListExecuteThread(buff.continuous, this, nullptr, vec3(0.f), buff.parameters, ins->lv).execute();
		//		ins->t0 = ins->timer;
		//	}

		//	if (ins->timer > 0)
		//	{
		//		ins->timer -= delta_time;
		//		if (ins->timer <= 0)
		//		{
		//			it = buffs.erase(it);
		//			stats_dirty = true;
		//			continue;
		//		}
		//	}
		//	it++;
		//}
		for (auto it = markers.begin(); it != markers.end();)
		{
			if (it->second.first > 0.f)
			{
				it->second.first -= delta_time;
				if (it->second.first <= 0.f)
				{
					it = markers.erase(it);
					continue;
				}
			}
			it++;
		}

		switch (command)
		{
		case CommandIdle:
			action = CharacterActionNone;
			break;
		case CommandMoveTo:
			if (process_approach(target_location))
			{
				if (nav_agent)
					nav_agent->stop();
				cmd_idle();
			}
			break;
		case CommandAttackTarget:
			if (!target.comp)
				cmd_idle();
			else
				process_attack_target(target.get<cCharacterPtr>());
			break;
		case CommandAttackLocation:
			if (!target.comp)
			{
				if (search_timer <= 0.f)
				{
					auto enemies = find_characters_within_circle(~faction, node->pos, max(atk_distance, 5.f));
					if (!enemies.empty())
						target.set(enemies.front());

					search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
				}
			}

			if (target.comp)
				process_attack_target(target.get<cCharacterPtr>());
			else
			{
				if (process_approach(target_location))
					action = CharacterActionNone;
			}
			break;
		case CommandHold:
			if (!target.comp)
			{
				if (search_timer <= 0.f)
				{
					auto enemies = find_characters_within_circle(~faction, node->pos, max(atk_distance, 1.5f));
					if (!enemies.empty())
						target.set(enemies.front());

					search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
				}
			}

			if (target.comp)
				process_attack_target(target.get<cCharacterPtr>(), false);
			else
				action = CharacterActionNone;
			break;
		case CommandInteract:
			if (!target.comp)
				cmd_idle();
			else
			{
				switch (target.comp->type_hash)
				{
				case "cChest"_h:
					auto chest = target.get<cChestPtr>();
					if (process_approach(chest->node->pos, 1.5f))
					{
						if (gain_item(chest->item_id, chest->item_num))
							chest->die();

						if (nav_agent)
							nav_agent->stop();
						cmd_idle();
					}
					break;
				}
			}
			break;
		case CommandCastAbility:
			process_cast_ability((cAbilityPtr)target_obj, vec3(0.f), nullptr);
			break;
		case CommandCastAbilityToLocation:
			process_cast_ability((cAbilityPtr)target_obj, target_location, nullptr);
			break;
		case CommandCastAbilityToTarget:
			if (!target.comp)
				cmd_idle();
			else
				process_cast_ability((cAbilityPtr)target_obj, vec3(0.f), target.get<cCharacterPtr>());
			break;
		}
	}

	if (armature)
	{
		if (dead)
		{
			armature->loop = false;
			armature->playing_speed = 1.f;
			armature->play("die"_h);
		}
		else
		{
			switch (action)
			{
			case CharacterActionNone:
				armature->loop = true;
				armature->playing_speed = 1.f;
				armature->play("idle"_h);
				break;
			case CharacterActionMove:
				armature->loop = true;
				armature->playing_speed = move_speed;
				armature->play("run"_h);
				break;
			case CharacterActionAttack:
				armature->loop = false;
				armature->playing_speed = attack_speed;
				armature->play("attack"_h);
				break;
			case CharacterActionCast:
				armature->loop = false;
				armature->playing_speed = cast_speed;
				armature->play("cast"_h);
				break;
			}
		}
	}
}

void cCharacter::die(uint type)
{
	if (dead)
		return;

	set_hp(0);

	if (!drop_items.empty())
	{
		std::vector<std::pair<uint, uint>> drops;
		for (auto& d : drop_items)
		{
			if (linearRand(0U, 100U) < d.probability)
				drops.emplace_back(d.id, linearRand(d.num_min, d.num_max));
		}
		auto p = node->pos;
		for (auto& d : drops)
			add_chest(get_map_coord(p + vec3(linearRand(-0.3f, +0.3f), 0.f, linearRand(-0.3f, +0.3f))), d.first, d.second);
	}

	dead_characters.push_back(this);
	dead = true;
}

void cCharacter::inflict_damage(cCharacterPtr target, DamageType type, uint value)
{
	value *= linearRand(0.8f, 1.2f);
	uint def;
	switch (type)
	{
	case PhysicalDamage:
		def = target->phy_def;
		break;
	case MagicDamage:
		def = target->mag_def;
		break;
	}
	;
	if (target->take_damage(type, value))
		gain_exp(target->exp_max * 0.15f);
}

bool cCharacter::take_damage(DamageType type, uint value)
{
	if (dead)
		return false;
	if (hp > value)
	{
		set_hp(hp - value);
		return false;
	}

	die("no_hp"_h);
	return true;
}

void cCharacter::restore_hp(uint value)
{
	set_hp(min(hp + value, hp_max));
}

void cCharacter::restore_mp(uint value)
{
	set_hp(min(mp + value, mp_max));
}

void cCharacter::gain_exp(uint v)
{
	if (exp_max == 0)
		return;
	exp += v;

	while (exp_max > 0 && exp >= exp_max)
	{
		exp -= exp_max;
		lv++;
		ability_points++;
		exp_max *= 1.1f;
		stats_dirty = true;

	}
}

bool cCharacter::gain_item(uint id, uint num)
{
	//for (auto i = 0; i < inventory.size(); i++)
	//{
	//	auto& ins = inventory[i];
	//	if (!ins)
	//	{
	//		ins.reset(new ItemInstance);
	//		ins->id = id;
	//		ins->num = num;
	//		stats_dirty = true;

	//		for (auto& cb : main_player.character->message_listeners.list)
	//			cb.first(CharacterGainItem, { .u = id }, { .u = num }, { .i = i }, {});

	//		return true;
	//	}
	//}

	return false;
}

bool cCharacter::gain_ability(uint id, uint lv)
{
	//for (auto& ins : abilities)
	//{
	//	if (ins->id == id)
	//		return false;
	//}

	//auto ins = new AbilityInstance;
	//ins->id = id;
	//ins->lv = lv;
	//abilities.emplace_back(ins);
	//stats_dirty = true;

	//for (auto& cb : main_player.character->message_listeners.list)
	//	cb.first(CharacterGainAbility, { .u = id }, { .u = lv }, { .i = (int)abilities.size() - 1 }, {});

	return true;
}

bool cCharacter::gain_talent(uint id)
{
	//for (auto tid : talents)
	//{
	//	if (tid == id)
	//		return false;
	//}

	//talents.push_back(id);
	//auto& talent = Talent::get(id);
	//for (auto& layer : talent.ablilities_list)
	//{
	//	for (auto id : layer)
	//		gain_ability(id, 0);
	//}
	return true;
}

void cCharacter::use_item(cItemPtr item)
{
	//auto& item = Item::get(ins->id);
	//if (item.active)
	//	return;
	//if (item.type == ItemConsumable)
	//{
	//	if (ins->num == 0)
	//		return;
	//	ins->num--;
	//	if (ins->num == 0)
	//	{
	//		for (auto i = 0; i < inventory.size(); i++)
	//		{
	//			if (inventory[i].get() == ins)
	//			{
	//				inventory[i].reset(nullptr);
	//				break;
	//			}
	//		}
	//	}
	//}

	//if (!item.active.cmds.empty())
	//	cl_threads.emplace_back(item.active, this, nullptr, vec3(0.f), item.parameters, 0);
}

void cCharacter::cast_ability(cAbilityPtr ability, const vec3& location, cCharacterPtr target)
{
	if (ability->cd_timer > 0.f)
		return;

	if (mp < ability->mp)
		return;

	ability->cd_timer = ability->cd_max = ability->cd_max;
	set_mp(mp - ability->mp);

	if (ability->active.type)
		ability->active.value().exec(ability, this, location, target);
}

void cCharacter::add_buff(uint id, float time, uint lv, bool replace)
{
	//BuffInstance* ins = nullptr;
	//if (replace)
	//{
	//	for (auto& i : buffs)
	//	{
	//		if (i->id == id)
	//		{
	//			ins = i.get();
	//			break;
	//		}
	//	}
	//}
	//if (!ins)
	//{
	//	ins = new BuffInstance;
	//	buffs.emplace_back(ins);
	//}
	//auto& buff = Buff::get(id);
	//ins->id = id;
	//ins->lv = lv;
	//ins->timer = time;
	//ins->t0 = time;
	//ins->interval = buff.interval;
	//ins->duration = time;
	//stats_dirty = true;
}

bool cCharacter::add_marker(uint hash, float time)
{
	auto it = markers.find(hash);
	if (it == markers.end())
	{
		markers.emplace(hash, std::make_pair(time, 0));
		return true;
	}
	return false;
}

bool cCharacter::process_approach(const vec3& target, float dist, float ang)
{
	if (state & CharacterStateStun)
	{
		if (nav_agent)
			nav_agent->stop();
		action = CharacterActionNone;
		return false;
	}
	if (nav_agent)
	{
		move_speed = max(0.1f, mov_sp / 100.f);
		nav_agent->set_target(target);
		nav_agent->set_speed_scale(move_speed);

		if ((nav_agent->reached_pos == target || nav_agent->dist < dist) && (ang <= 0.f || abs(nav_agent->ang_diff) < ang))
			return true;
		if (audio_source)
			audio_source->play("move"_h);
		action = CharacterActionMove;
	}
	else
	{
		move_speed = 0.f;
		action = CharacterActionNone;
	}
	return false;
}

static void attack_proc(cCharacterPtr character, cCharacterPtr target)
{
	auto damage = character->atk;
	character->inflict_damage(target, (DamageType)character->atk_type, damage);
	{
		//for (auto& ef : character->attack_effects)
		//	cl_threads.emplace_back(ef, character, target, vec3(0.f), parameters, 0);
	}

	if (character->audio_source)
		character->audio_source->play("attack_hit"_h);
}

void cCharacter::process_attack_target(cCharacterPtr target, bool chase_target)
{
	if (state & CharacterStateStun)
	{
		if (nav_agent)
			nav_agent->stop();
		action = CharacterActionNone;
		return;
	}

	auto p0 = node->pos;
	auto p1 = target->node->pos;

	if (action == CharacterActionAttack)
	{
		if (nav_agent)
		{
			nav_agent->set_speed_scale(0.f);
			nav_agent->set_target(p1);
		}

		if (action_timer >= 0.f)
		{
			action_timer += delta_time;
			if (pre_action_time > 0.f)
			{
				if (action_timer >= pre_action_time)
				{
					if (distance(p0, p1) <= atk_distance + 3.5f)
					{
						if (!atk_projectile.empty())
						{
							auto character = this;
							auto pj = add_projectile(atk_projectile, p0 + vec3(0.f, get_height() * 0.9f, 0.f), target, 6.f);
							pj->on_end = [character](const vec3&, cCharacterPtr target) {
								if (target)
									attack_proc(character, target);
							};
						}
						else
							attack_proc(this, target);
					}

					attack_interval_timer = (atk_interval - atk_point) / attack_speed;
					pre_action_time = 0.f;
				}
			}
			if (action_timer >= total_action_time)
				action = CharacterActionNone;
		}
	}
	else
	{
		float dist = atk_distance;
		if (last_approached)
			dist += 1.f;
		auto approached = false;
		if (chase_target && nav_agent)
			approached = process_approach(p1, dist, 60.f);
		else
		{
			if (nav_agent)
			{
				nav_agent->set_target(p1);
				nav_agent->set_speed_scale(0.f);

				approached = nav_agent->dist < dist && abs(nav_agent->ang_diff) < 60.f;
			}
			else
				approached = distance(p0, p1) < dist;
			action = CharacterActionNone;
		}
		last_approached = approached;
		if (attack_interval_timer <= 0.f)
		{
			if (approached)
			{
				attack_speed = max(0.01f, atk_sp / 100.f); 
				pre_action_time = atk_point / attack_speed;
				total_action_time = atk_time / attack_speed;
				action = CharacterActionAttack;
				action_timer = 0.f;

				if (audio_source)
					audio_source->play("attack_precast"_h);
			}
		}
		else
		{
			if (approached)
			{
				action = CharacterActionNone;
				if (nav_agent)
					nav_agent->set_speed_scale(0.f);
			}
		}
	}
}

void cCharacter::process_cast_ability(cAbilityPtr ability, const vec3& location, cCharacterPtr target)
{
	if (state & CharacterStateStun)
	{
		if (nav_agent)
			nav_agent->stop();
		action = CharacterActionNone;
		return;
	}

	auto p0 = node->pos;
	auto p1 = target ? target->node->pos : location;

	if (action == CharacterActionCast)
	{
		if (nav_agent)
		{
			nav_agent->set_speed_scale(0.f);
			nav_agent->set_target(p1);
		}

		if (action_timer >= 0.f)
		{
			action_timer += delta_time;
			if (pre_action_time > 0.f)
			{
				if (action_timer >= pre_action_time)
				{
					if (distance(p0, p1) <= ability->distance + 1.f)
					{
						cast_ability(ability, p1, target);
						nav_agent->stop();
						cmd_idle();
					}
					pre_action_time = 0.f;
				}
			}
			if (action_timer < total_action_time)
				action = CharacterActionNone;
		}
	}
	else
	{
		float dist = ability->distance;
		if (last_approached)
			dist += 1.f;
		auto approached = false;
		if (ability->target_type == TargetNull)
			approached = true;
		else
		{
			if (nav_agent)
				approached = process_approach(p1, dist, 15.f);
		}
		last_approached = approached;
		if (approached)
		{
			if (ability->cast_time == 0.f)
			{
				cast_ability(ability, p1, target);
				nav_agent->stop();
				cmd_idle();
			}
			else
			{
				cast_speed = 1.f;
				pre_action_time = ability->cast_point / attack_speed;
				total_action_time = ability->cast_time / attack_speed;
				action = CharacterActionCast;
				action_timer = 0.f;
			}
		}
	}
}

void cCharacter::reset_cmd()
{
	command = CommandIdle;
	target.reset();
	action = CharacterActionNone;
	action_timer = -1.f;
}

void cCharacter::cmd_idle()
{
	reset_cmd();
	command = CommandIdle;
}

void cCharacter::cmd_move_to(const vec3& location)
{
	reset_cmd();
	command = CommandMoveTo;
	target_location = location;
}

void cCharacter::cmd_attack_target(cCharacterPtr _target)
{
	reset_cmd();
	command = CommandAttackTarget;
	target.set(_target);
}

void cCharacter::cmd_attack_location(const vec3& location)
{
	reset_cmd();
	command = CommandAttackLocation;
	target_location = location;
}

void cCharacter::cmd_hold()
{
	reset_cmd();
	command = CommandHold;
}

void cCharacter::cmd_interact(ComponentPtr _target)
{
	reset_cmd();
	command = CommandInteract;
	target.set(_target);
}

void cCharacter::cmd_cast_ability(cAbilityPtr ability)
{
	reset_cmd();
	command = CommandCastAbility;
	target_obj = ability;
}

void cCharacter::cmd_cast_ability_to_location(cAbilityPtr ability, const vec3& location)
{
	reset_cmd();
	command = CommandCastAbilityToLocation;
	target_obj = ability;
	target_location = location;
}

void cCharacter::cmd_cast_ability_to_target(cAbilityPtr ability, cCharacterPtr _target)
{
	reset_cmd();
	command = CommandCastAbilityToTarget;
	target_obj = ability;
	target.set(_target);
}

struct cCharacterCreate : cCharacter::Create
{
	cCharacterPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cCharacter;
	}
}cCharacter_create;
cCharacter::Create& cCharacter::create = cCharacter_create;
