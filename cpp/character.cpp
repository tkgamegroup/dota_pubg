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
	character->nav_timer = 0.f;
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
	character->nav_timer = 0.f;
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
	character->nav_timer = 0.f;
}

void CommandAttackLocation::update()
{
	auto atk_dist = character->get_preset().atk_distance;
	auto dist = target.obj ? distance(character->node->pos, target.obj->node->pos) : 10000.f;
	if (dist > atk_dist + 12.f)
		target.set(nullptr);
	if (character->action != ActionAttack)
	{
		if (character->search_timer <= 0.f)
		{
			auto enemies = get_characters(character->node->g_pos, 3.5f, ~character->faction);
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
	character->nav_timer = 0.f;
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
		preset.exp_list = { 200, 0 };
		preset.hp = 200;
		preset.mp = 75;
		preset.STR = 21;
		preset.AGI = 16;
		preset.INT = 18;
		preset.STR_GROW = 3;
		preset.AGI_GROW = 2;
		preset.INT_GROW = 1;
		preset.atk = 32;
		preset.atk_time = 1.5f;
		preset.atk_point = 0.68f;
		preset.cast_time = 0.5f;
		preset.cast_point = 0.3f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Blood Seeker";
		preset.exp_list = { 200, 0 };
		preset.hp = 200;
		preset.mp = 75;
		preset.STR = 24;
		preset.AGI = 22;
		preset.INT = 17;
		preset.STR_GROW = 2;
		preset.AGI_GROW = 3;
		preset.INT_GROW = 2;
		preset.atk = 35;
		preset.atk_time = 2.6f;
		preset.atk_point = 1.5f;
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

void cCharacter::set_preset_name(const std::string& name)
{
	if (preset_name == name)
		return;
	preset_name = name;
	preset_id = CharacterPreset::find(preset_name);
}

cCharacter::~cCharacter()
{
	node->measurers.remove("character"_h);

	graphics::gui_callbacks.remove((uint)this);
}

void cCharacter::on_init()
{
	node->measurers.add([this](AABB* ret) {
		auto radius = nav_agent->radius;
		auto height = nav_agent->height;
		*ret = AABB(AABB(vec3(-radius, 0.f, -radius), vec3(radius, height, radius)).get_points(node->transform));
		return true;
	}, "character"_h);
}

static uint calc_exp(uint lv)
{
	return lv * 10;
}

void cCharacter::inflict_damage(cCharacterPtr target, uint value)
{
	if (target->take_damage(value))
		gain_exp(calc_exp(target->lv));
}

bool cCharacter::take_damage(uint value)
{
	if (dead)
		return false;
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
		exp_max = get_preset().exp_list[lv - 1];
		stats_dirty = true;

		if (lv == 2)
			gain_ability(Ability::find("Blink"));
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

bool cCharacter::gain_ability(uint id)
{
	for (auto& ins : abilities)
	{
		if (!ins)
		{
			ins.reset(new AbilityInstance);
			ins->id = id;
			stats_dirty = true;
			return true;
		}
	}
	return false;
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
			for (auto i = 0; i < countof(inventory); i++)
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
	if (mp < ability.mana)
		return;
	if (ability.active)
		ability.active(this);
	else if (ability.active_l)
		ability.active_l(this, location);
	else if (ability.active_t)
		ability.active_t(this, target);
	ins->cd_max = ability.cd;
	ins->cd_timer = ins->cd_max;
	mp -= ability.mana;
}

void cCharacter::add_buff(uint id, float time)
{
	auto ins = new BuffInstance;
	ins->id = id;
	ins->timer = time;
	buffs.emplace_back(ins);
	stats_dirty = true;
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

	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		if (main_camera.camera)
		{
			auto p = main_camera.camera->world_to_screen(node->pos + vec3(0.f, nav_agent->height + 0.1f, 0.f));
			if (p.x > 0.f && p.y > 0.f)
			{
				p += sInput::instance()->offset;
				auto dl = ImGui::GetForegroundDrawList();
				const auto bar_width = 80.f;
				const auto bar_height = 5.f;
				p.x -= bar_width * 0.5f;
				dl->AddRectFilled(p, p + vec2((float)hp / (float)hp_max * bar_width, bar_height),
					faction == main_player.character->faction ? ImColor(0.f, 1.f, 0.f) : ImColor(1.f, 0.f, 0.f));
			}
		}
	}, (uint)this);
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
	if (nav_timer <= 0.f)
	{
		nav_agent->set_target(target);
		nav_timer = linearRand(0.09f, 0.11f);
	}
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

	if (attack_timer > 0.f)
	{
		attack_timer -= delta_time;
		if (attack_timer <= 0.f)
		{
			if (preset.atk_projectile)
			{
				add_projectile(preset.atk_projectile, 
					node->pos + vec3(0.f, nav_agent->height * 0.5f, 0.f), 
					target, 0.1f, 
				[this](cCharacterPtr t) {
					if (t) 
						t->take_damage(atk);
				});
			}
			else
				target->take_damage(atk);

			attack_interval_timer = (preset.atk_time - preset.atk_point) / attack_speed;

			action = ActionNone;
		}
		action = ActionAttack;
		nav_agent->set_speed_scale(0.f);
	}
	else
	{
		if (attack_interval_timer <= 0.f)
		{
			if (process_approach(target->node->pos, preset.atk_distance, 60.f))
			{
				attack_speed = max(0.01f, atk_sp / 100.f);
				attack_timer = preset.atk_point / attack_speed;
				nav_agent->set_speed_scale(0.f);
			}
		}
		else
			action = ActionAttack;
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

	if (cast_timer > 0.f)
	{
		cast_timer -= delta_time;
		if (cast_timer <= 0.f)
		{
			cast_ability(ins, pos, target);
			nav_agent->stop();
			new CommandIdle(this);
			return;
		}
		action = ActionCast;
		nav_agent->set_speed_scale(0.f);
	}
	else
	{
		if (process_approach(pos, ability.distance, 15.f))
		{
			if (ability.cast_time == 0.f)
			{
				cast_ability(ins, pos, target);
				nav_agent->stop();
				new CommandIdle(this);
				return;
			}
			cast_speed = preset.cast_time / ability.cast_time;
			cast_timer = preset.cast_point / cast_speed;
			nav_agent->set_speed_scale(0.f);
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

		exp_max = preset.exp_list.empty() ? 0 : preset.exp_list[lv - 1];

		state = StateNormal;

		auto pre_hp_max = hp_max;
		auto pre_mp_max = mp_max;

		hp_max = preset.hp;
		mp_max = preset.mp;

		STR = preset.STR + round(preset.STR_GROW * (lv - 1));
		AGI = preset.AGI + round(preset.AGI_GROW * (lv - 1));
		INT = preset.INT + round(preset.INT_GROW * (lv - 1));

		atk = preset.atk;

		mov_sp = preset.mov_sp;
		atk_sp = preset.atk_sp;

		for (auto& ins : inventory)
		{
			if (ins)
			{
				auto& item = Item::get(ins->id);
				if (item.passive)
					item.passive(this);
			}
		}
		for (auto& ins : abilities)
		{
			if (ins)
			{

			}
		}
		for (auto& ins : buffs)
		{
			auto& buff = Buff::get(ins->id);
			if (buff.passive)
				buff.passive(this);
		}

		hp_max += STR * 20;
		mp_max += INT * 12;

		if (hp_max != pre_hp_max)
			hp *= (float)hp_max / pre_hp_max;
		if (mp_max != pre_mp_max)
			mp *= (float)mp_max / pre_mp_max;

		atk += STR;

		stats_dirty = false;
	}

	if (search_timer > 0)
		search_timer -= delta_time;
	if (nav_timer > 0)
		nav_timer -= delta_time;
	if (attack_interval_timer > 0)
		attack_interval_timer -= delta_time;

	for (auto& ins : abilities)
	{
		if (ins && ins->cd_timer > 0.f)
			ins->cd_timer -= delta_time;
	}

	for (auto it = buffs.begin(); it != buffs.end();)
	{
		if ((*it)->timer > 0)
		{
			(*it)->timer -= delta_time;
			if ((*it)->timer <= 0)
			{
				it = buffs.erase(it);
				stats_dirty = true;
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
			armature->playing_speed = 1.f;
			armature->play("idle"_h);
			break;
		case ActionMove:
			armature->playing_speed = move_speed;
			armature->play("run"_h);
			break;
		case ActionAttack:
			armature->playing_speed = attack_speed;
			armature->play("attack"_h);
			break;
		case ActionCast:
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
