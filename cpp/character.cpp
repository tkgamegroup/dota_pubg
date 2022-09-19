#include "character.h"
#include "item.h"
#include "ability.h"
#include "buff.h"
#include "projectile.h"
#include "chest.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

Command::Command(cCharacterPtr character) :
	character(character)
{
	character->command.reset(this);
	character->action = ActionNone;
}

CommandIdle::CommandIdle(cCharacterPtr character) :
	Command(character)
{
}

void CommandIdle::update()
{
	character->action = ActionNone;
}

CommandMoveTo::CommandMoveTo(cCharacterPtr character, const vec3& _location) :
	Command(character)
{
	location = _location;
}

void CommandMoveTo::update()
{
	if (character->process_approach(location, 0.6f))
	{
		character->nav_agent->stop();
		new CommandIdle(character);
	}
}

CommandAttackTarget::CommandAttackTarget(cCharacterPtr character, cCharacterPtr _target) :
	Command(character)
{
	target.set(_target);
}

void CommandAttackTarget::update()
{
	if (!target.obj)
		new CommandIdle(character);
	else
		character->process_attack_target(target.obj);
}

CommandAttackLocation::CommandAttackLocation(cCharacterPtr character, const vec3& _location) :
	Command(character)
{
	location = _location;
}

void CommandAttackLocation::update()
{
	auto atk_dist = character->get_preset().atk_distance;
	auto dist = target.obj ? distance(character->node->pos, target.obj->node->pos) : 10000.f;
	if (dist > atk_dist + 10.f)
		target.set(nullptr);
	if (character->action != ActionAttack)
	{
		if (character->search_timer <= 0.f)
		{
			auto enemies = get_characters(character->node->pos, 5.f, ~character->faction);
			if (!enemies.empty() && dist > atk_dist)
				target.set(enemies.front());

			character->search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
		}
	}

	if (target.obj)
		character->process_attack_target(target.obj);
	else
		character->process_approach(location);
}

CommandPickUp::CommandPickUp(cCharacterPtr character, cChestPtr _target) :
	Command(character)
{
	target.set(_target);
}

void CommandPickUp::update()
{
	if (!target.obj)
		new CommandIdle(character);
	else
	{
		if (character->process_approach(target.obj->node->pos, 1.f))
		{
			if (character->gain_item(target.obj->item_id, target.obj->item_num))
			{
				auto e = target.obj->entity;
				add_event([e]() {
					e->remove_from_parent();
					return false;
				});
			}

			character->nav_agent->stop();
			new CommandIdle(character);
		}
	}
}

CommandCastAbilityToLocation::CommandCastAbilityToLocation(cCharacterPtr character, AbilityInstance* ins, const vec3& _location) :
	Command(character),
	ins(ins)
{
	location = _location;
}

void CommandCastAbilityToLocation::update()
{
	character->process_cast_ability(ins, location, nullptr);
}

CommandCastAbilityToTarget::CommandCastAbilityToTarget(cCharacterPtr character, AbilityInstance* ins, cCharacterPtr _target) :
	Command(character),
	ins(ins)
{
	target.set(_target);
}

void CommandCastAbilityToTarget::update()
{
	if (!target.obj)
		new CommandIdle(character);
	else
		character->process_cast_ability(ins, vec3(0.f), target.obj);
}

std::vector<CharacterPreset> character_presets;

void load_character_presets()
{
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Dragon Knight";
		preset.exp_base = 200;
		preset.hp = 2000;
		preset.mp = 500;
		preset.VIG = 20;
		preset.MND = 10;
		preset.STR = 21;
		preset.DEX = 16;
		preset.INT = 10;
		preset.LUK = 10;
		preset.atk = 32;
		preset.atk_time = 1.7f;
		preset.atk_point = 0.5f;
		preset.cast_time = 0.5f;
		preset.cast_point = 0.3f;
		preset.abilities.emplace_back("Fire Thrower", 0);
		preset.abilities.emplace_back("Shield Bash", 0);
		preset.abilities.emplace_back("Flame Weapon", 0);
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Life Stealer";
		preset.exp_base = 200;
		preset.hp = 2000;
		preset.mp = 500;
		preset.VIG = 20;
		preset.MND = 10;
		preset.STR = 24;
		preset.DEX = 22;
		preset.INT = 10;
		preset.LUK = 10;
		preset.atk = 35;
		preset.atk_time = 1.7f;
		preset.atk_point = 0.39f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Slark";
		preset.exp_base = 200;
		preset.hp = 2000;
		preset.mp = 500;
		preset.VIG = 20;
		preset.MND = 10;
		preset.STR = 20;
		preset.DEX = 21;
		preset.INT = 10;
		preset.LUK = 10;
		preset.atk = 37;
		preset.atk_time = 1.7f;
		preset.atk_point = 0.5f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Spiderling";
		preset.exp_base = 200;
		preset.hp = 4500;
		preset.mp = 0;
		preset.atk = 14;
		preset.atk_time = 1.35f;
		preset.atk_point = 0.5f;
		preset.hp_reg = 5;
		preset.mov_sp = 400;
		preset.abilities.emplace_back("Stinger", 1);
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Treant";
		preset.exp_base = 200;
		preset.hp = 5500;
		preset.mp = 0;
		preset.atk = 15;
		preset.atk_time = 1.6f;
		preset.atk_point = 0.467f;
		preset.hp_reg = 25;
		preset.mov_sp = 100;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Boar";
		preset.exp_base = 200;
		preset.hp = 3000;
		preset.mp = 1000;
		preset.atk = 20;
		preset.atk_time = 1.25f;
		preset.atk_point = 0.5f;
		preset.hp_reg = 15;
		preset.mov_sp = 100;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Creep";
		preset.exp_base = 200;
		preset.hp = 5000;
		preset.mp = 0;
		preset.atk = 21;
		preset.atk_time = 1.7f;
		preset.atk_point = 0.39f;
		preset.hp_reg = 5;
	}
}

int CharacterPreset::find(const std::string& name)
{
	if (character_presets.empty())
		load_character_presets();
	for (auto i = 0; i < character_presets.size(); i++)
	{
		if (character_presets[i].name == name)
			return i;
	}
	return -1;
}

const CharacterPreset& CharacterPreset::get(uint id)
{
	if (character_presets.empty())
		load_character_presets();
	return character_presets[id];
}

void cCharacter::set_faction(uint _faction)
{
	if (faction == _faction)
		return;
	faction = _faction;
	if (nav_agent)
		nav_agent->separation_group = faction;
}

void cCharacter::set_preset_name(const std::string& name)
{
	if (preset_name == name)
		return;
	preset_name = name;
	preset_id = CharacterPreset::find(preset_name);

	auto& preset = get_preset();
	if (!preset.abilities.empty())
	{
		abilities.clear();
		for (auto& i : preset.abilities)
			gain_ability(Ability::find(i.first), i.second);
	}
}

cCharacter::~cCharacter()
{
	node->measurers.remove("character"_h);

	if (armature)
		armature->playing_callbacks.remove("character"_h);

	graphics::gui_callbacks.remove((uint)this);
}

void cCharacter::on_init()
{
	nav_agent->separation_group = faction;

	node->measurers.add([this](AABB* ret) {
		auto radius = nav_agent->radius;
		auto height = nav_agent->height;
		*ret = AABB(AABB(vec3(-radius, 0.f, -radius), vec3(radius, height, radius)).get_points(node->transform));
		return true;
	}, "character"_h);
}

void cCharacter::inflict_damage(cCharacterPtr target, uint value, DamageType type)
{
	if (target->take_damage(value, type))
		gain_exp(target->exp_max * 0.15f);
}

bool cCharacter::take_damage(uint value, DamageType type)
{
	if (dead)
		return false;
	value *= 10;
	if (hp > value)
	{
		hp -= value;
		return false;
	}

	die();
	return true;
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
		attribute_points += 5;
		ability_points++;
		exp_max *= 1.1f;
		stats_dirty = true;
	}
}

bool cCharacter::gain_item(uint id, uint num)
{
	for (auto& ins : inventory)
	{
		if (!ins)
		{
			ins.reset(new ItemInstance);
			ins->id = id;
			ins->num = num;
			stats_dirty = true;
			return true;
		}
	}
	return false;
}

bool cCharacter::gain_ability(uint id, uint lv)
{
	auto ins = new AbilityInstance;
	ins->id = id;
	ins->lv = lv;
	abilities.emplace_back(ins);
	stats_dirty = true;
	return true;
}

void cCharacter::use_item(ItemInstance* ins)
{
	auto& item = Item::get(ins->id);
	if (!item.active)
		return;
	if (item.type == ItemConsumable)
	{
		if (ins->num == 0)
			return;
		ins->num--;
		if (ins->num == 0)
		{
			for (auto i = 0; i < inventory.size(); i++)
			{
				if (inventory[i].get() == ins)
				{
					inventory[i].reset(nullptr);
					break;
				}
			}
		}
	}
	item.active(this);
}

void cCharacter::cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target)
{
	if (ins->cd_timer > 0.f)
		return;
	auto& ability = Ability::get(ins->id);
	if (mp < ability.mp)
		return;
	if (ability.cast_check)
	{
		if (!ability.cast_check(this))
			return;
	}
	if (ability.active)
		ability.active(this);
	else if (ability.active_l)
		ability.active_l(this, location);
	else if (ability.active_t)
		ability.active_t(this, target);
	ins->cd_max = ability.cd;
	ins->cd_timer = ins->cd_max;
	mp -= ability.mp;
}

void cCharacter::add_buff(uint id, float time, bool replace)
{
	BuffInstance* ins = nullptr;
	if (replace)
	{
		for (auto& i : buffs)
		{
			if (i->id == id)
			{
				ins = i.get();
				break;
			}
		}
	}
	if (!ins)
	{
		ins = new BuffInstance;
		buffs.emplace_back(ins);
	}
	ins->id = id;
	ins->timer = time;
	if (auto& buff = Buff::get(id); buff.start)
		buff.start(this, ins);
	stats_dirty = true;
}

bool cCharacter::add_marker(uint hash, float time)
{
	auto it = markers.find(hash);
	if (it == markers.end())
	{
		markers.emplace(hash, time);
		return true;
	}
	return false;
}

void cCharacter::die()
{
	if (dead)
		return;
	hp = 0;
	dead = true;
}

void cCharacter::start()
{
	entity->tag |= CharacterTag;

	auto e = entity;
	while (e)
	{
		if (armature = e->get_component_t<cArmature>(); armature)
			break;
		if (e->children.empty())
			break;
		e = e->children[0].get();
	}
	if (armature)
	{
		armature->playing_callbacks.add([this](uint ev, uint anim) {
			if (ev == "end"_h)
			{
				switch (anim)
				{
				case "attack"_h:
					attack_timer = -2.f;
					action = ActionNone;
					break;
				case "cast"_h:
					cast_timer = -2.f;
					action = ActionNone;
					break;
				}
			}
		}, "character"_h);
	}

	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		if (main_camera.camera)
		{
			auto p = main_camera.camera->world_to_screen(node->pos + vec3(0.f, nav_agent->height + 0.1f, 0.f));
			if (p.x > 0.f && p.y > 0.f)
			{
				p += sInput::instance()->offset;
				auto dl = ImGui::GetBackgroundDrawList();
				const auto bar_width = 80.f * (nav_agent->radius / 0.6f);
				const auto bar_height = 5.f;
				p.x -= bar_width * 0.5f;
				dl->AddRectFilled(p, p + vec2((float)hp / (float)hp_max * bar_width, bar_height),
					faction == main_player.character->faction ? ImColor(0.f, 1.f, 0.f) : ImColor(1.f, 0.f, 0.f));
			}
		}
	}, (uint)this);

	inventory.resize(16);
}

bool cCharacter::process_approach(const vec3& target, float dist, float ang)
{
	if (state & StateStun)
	{
		nav_agent->stop();
		action = ActionNone;
		return false;
	}

	move_speed = max(0.1f, mov_sp / 100.f);
	nav_agent->set_target(target);
	nav_agent->set_speed_scale(move_speed);

	if (nav_agent->dist <= dist && (ang == 0.f || abs(nav_agent->ang_diff) <= ang))
		return true;
	action = ActionMove;
	return false;
}

void cCharacter::process_attack_target(cCharacterPtr target)
{
	if (state & StateStun)
	{
		attack_timer = -1.f;
		nav_agent->stop();
		action = ActionNone;
		return;
	}

	auto& preset = get_preset();

	auto approached = process_approach(target->node->pos, preset.atk_distance, 60.f);
	if (attack_timer > 0.f)
	{
		attack_timer -= delta_time;
		if (attack_timer <= 0.f)
		{
			if (distance(node->pos, target->node->pos) <= preset.atk_distance + 3.5f)
			{
				auto attack = [this](cCharacterPtr target) {
					inflict_damage(target, atk, atk_type);
					for (auto& ef : attack_effects.list)
					{
						if (!target->dead)
							ef.first(this, target, atk_type, atk);
					}
				};
				if (preset.atk_projectile)
				{
					add_projectile(preset.atk_projectile,
						node->pos + vec3(0.f, nav_agent->height * 0.5f, 0.f), target, 6.f,
						[&](const vec3&, cCharacterPtr t) {
							if (t)
								attack(t);
						});
				}
				else
					attack(target);
			}

			attack_interval_timer = (preset.atk_time - preset.atk_point) / attack_speed;
		}
		action = ActionAttack;
		nav_agent->set_speed_scale(0.f);
	}
	else
	{
		if (attack_interval_timer <= 0.f)
		{
			if (approached)
			{
				attack_speed = max(0.01f, atk_sp / 100.f); 
				attack_timer = preset.atk_point / attack_speed;
			}
		}
		else
		{
			if (attack_timer > -1.f)
			{
				action = ActionAttack;
				nav_agent->set_speed_scale(0.f);
			}
			if (approached)
				nav_agent->set_speed_scale(0.f);
		}
	}
}

void cCharacter::process_cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target)
{
	if (state & StateStun)
	{
		cast_timer = -1.f;
		nav_agent->stop();
		action = ActionNone;
		return;
	}

	auto& preset = get_preset();
	auto& ability = Ability::get(ins->id);
	auto pos = target ? target->node->pos : location;

	auto approached = process_approach(pos, ability.distance, 15.f);
	if (cast_timer > 0.f)
	{
		cast_timer -= delta_time;
		if (cast_timer <= 0.f)
		{
			cast_ability(ins, pos, target);
			nav_agent->stop();
			new CommandIdle(this);
		}
		else
		{
			action = ActionCast;
			nav_agent->set_speed_scale(0.f);
		}
	}
	else
	{
		if (approached)
		{
			if (ability.cast_time == 0.f)
			{
				cast_ability(ins, pos, target);
				nav_agent->stop();
				new CommandIdle(this);
			}
			else
			{
				cast_speed = preset.cast_time / ability.cast_time;
				cast_timer = preset.cast_point / cast_speed;
			}
		}
	}
}

void cCharacter::update()
{
	if (dead)
	{
		add_event([this]() {
			entity->parent->remove_child(entity);
			return false;
		});
		return;
	}

	if (stats_dirty)
	{
		auto& preset = get_preset();

		if (exp_max == 0)
			exp_max = preset.exp_base;

		state = StateNormal;

		auto pre_hp_max = hp_max;
		auto pre_mp_max = mp_max;

		hp_max = preset.hp;
		mp_max = preset.mp;

		VIG = preset.VIG + VIG_PTS;
		MND = preset.MND + MND_PTS;
		STR = preset.STR + STR_PTS;
		DEX = preset.DEX + DEX_PTS;
		INT = preset.INT + INT_PTS;
		LUK = preset.LUK + LUK_PTS;

		atk_type = PhysicalDamage;
		atk = preset.atk;

		hp_reg = preset.hp_reg;
		mp_reg = preset.mp_reg;
		mov_sp = preset.mov_sp;
		atk_sp = preset.atk_sp;

		attack_effects.list.clear();
		injury_effects.list.clear();
		for (auto& ins : abilities)
		{
			if (ins && ins->lv > 0)
			{
				auto& ability = Ability::get(ins->id);
				if (ability.passive)
					ability.passive(this);
			}
		}
		for (auto& ins : equipments)
		{
			if (ins.id != -1)
			{
				auto& item = Item::get(ins.id);
				if (item.passive)
					item.passive(this);
				if (ins.enchant != -1)
				{
					auto& buff = Buff::get(ins.enchant);
					if (buff.passive)
						buff.passive(this, nullptr);
				}
			}
		}
		for (auto& ins : buffs)
		{
			auto& buff = Buff::get(ins->id);
			if (buff.passive)
				buff.passive(this, ins.get());
		}

		hp_max += VIG * 200;
		hp_reg += VIG;
		mp_max += MND * 200;
		mp_reg += MND;

		if (hp_max != pre_hp_max)
			hp *= (float)hp_max / pre_hp_max;
		if (mp_max != pre_mp_max)
			mp *= (float)mp_max / pre_mp_max;

		if (equipments[EquipWeapon0].id == -1)
			atk += STR;

		stats_dirty = false;
	}

	if (regeneration_timer > 0)
		regeneration_timer -= delta_time;
	else
	{
		hp += hp_reg;
		if (hp > hp_max) hp = hp_max;
		mp += mp_reg;
		if (mp > mp_max) mp = mp_max;
		regeneration_timer = 1.f;
	}

	if (search_timer > 0)
		search_timer -= delta_time;
	if (attack_interval_timer > 0)
		attack_interval_timer -= delta_time;

	for (auto& ins : abilities)
	{
		if (ins && ins->cd_timer > 0.f)
			ins->cd_timer -= delta_time;
	}
	for (auto& ins : equipments)
	{
		if (ins.id != -1)
		{
			if (ins.enchant != -1)
			{
				ins.enchant_timer -= delta_time;
				if (ins.enchant_timer <= 0)
				{
					ins.enchant = -1;
					stats_dirty = true;
				}
			}
		}
	}
	for (auto it = buffs.begin(); it != buffs.end();)
	{
		auto& ins = *it;
		auto& buff = Buff::get(ins->id);
		if (buff.continuous)
			buff.continuous(this, ins.get());

		if (ins->timer > 0)
		{
			ins->timer -= delta_time;
			if (ins->timer <= 0)
			{
				it = buffs.erase(it);
				stats_dirty = true;
				continue;
			}
		}
		it++;
	}
	for (auto it = markers.begin(); it != markers.end();)
	{
		if (it->second > 0)
		{
			it->second -= delta_time;
			if (it->second <= 0)
			{
				it = markers.erase(it);
				continue;
			}
		}
		it++;
	}

	if (command)
		command->update();

	if (armature)
	{
		switch (action)
		{
		case ActionNone:
			armature->loop = true;
			armature->playing_speed = 1.f;
			armature->play("idle"_h);
			break;
		case ActionMove:
			armature->loop = true;
			armature->playing_speed = move_speed;
			armature->play("run"_h);
			break;
		case ActionAttack:
			armature->loop = false;
			armature->playing_speed = attack_speed;
			armature->play("attack"_h);
			break;
		case ActionCast:
			armature->loop = false;
			armature->playing_speed = 1.f;
			armature->play("cast"_h);
			break;
		}
	}
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
