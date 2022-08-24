#include "character.h"
#include "item.h"
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
	auto dist = target.obj ? distance(character->node->g_pos, target.obj->node->g_pos) : 10000.f;
	if (dist > atk_dist + 12.f)
		target.set(nullptr);
	if (character->action != ActionAttack)
	{
		auto enemies = character->find_enemies(0.f, false, true);
		if (!enemies.empty() && dist > atk_dist)
			target.set(enemies.front());
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
		if (character->process_approach(target.obj->node->g_pos, 1.f))
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

CommandCast::CommandCast(cCharacterPtr character, cCharacterPtr _target) :
	Command(character)
{
	target.set(_target);
}

void CommandCast::update()
{
	if (!target.obj)
		new CommandIdle(character);
	else
		;
}

std::vector<CharacterPreset> character_presets;

const CharacterPreset& CharacterPreset::get(uint id)
{
	assert(id < character_presets.size());
	return character_presets[id];
}

void load_character_presets()
{
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Knight";
		preset.exp = 200;
		preset.atk_interval = 1.8f;
		preset.atk_precast = 0.43f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Mutant";
		preset.atk_interval = 3.f;
		preset.atk_precast = 0.49f;
	}
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

std::vector<cCharacterPtr> cCharacter::find_enemies(float radius, bool ignore_timer, bool sort)
{
	std::vector<cCharacterPtr> ret;
	if (!ignore_timer)
	{
		if (search_timer <= 0.f)
			search_timer = 0.1f + linearRand(0.f, 0.05f);
		else
			return ret;
	}
	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(node->g_pos, radius ? radius : 3.5f, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && chr->faction != faction)
			ret.push_back(chr);
	}
	if (sort)
	{
		std::vector<std::pair<float, cCharacterPtr>> dist_list(ret.size());
		auto self_pos = node->g_pos;
		for (auto i = 0; i < ret.size(); i++)
		{
			auto c = ret[i];
			dist_list[i] = std::make_pair(distance(c->node->g_pos, self_pos), c);
		}
		std::sort(dist_list.begin(), dist_list.end(), [](const auto& a, const auto& b) {
			return a.first < b.first;
		});
		for (auto i = 0; i < ret.size(); i++)
			ret[i] = dist_list[i].second;
	}
	return ret;
}

const auto NextLvExpFactor = 1.1f;

static uint calc_exp(uint lv)
{
	return (100.f * (1.f - pow(NextLvExpFactor, lv)) / (1.f - NextLvExpFactor)) * 0.13f;
}

void cCharacter::inflict_damage(cCharacterPtr target, uint value)
{
	if (target->take_damage(value))
	{
		if (exp_max > 0)
			gain_exp(calc_exp(target->lv));
	}
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
	exp += v;
	while (exp >= exp_max)
	{
		exp -= exp_max;
		lv++;
		exp_max *= NextLvExpFactor;

		if (lv == 2)
			gain_ability(0);
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
			auto p = main_camera.camera->world_to_screen(node->g_pos + vec3(0.f, nav_agent->height + 0.1f, 0.f));
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
	auto& preset = get_preset();

	if (process_approach(target->node->g_pos, preset.atk_distance, 60.f))
	{
		if (action == ActionNone || action == ActionMove)
		{
			if (attack_interval_timer <= 0.f)
			{
				action = ActionAttack;
				attack_speed = max(0.01f, atk_sp / 100.f);
				attack_interval_timer = preset.atk_interval / attack_speed;
				attack_timer = attack_interval_timer * preset.atk_precast;
			}
			else
				action = ActionNone;
			nav_agent->set_speed_scale(0.f);
		}
	}

	if (action == ActionAttack)
	{
		if (attack_timer <= 0.f)
		{
			if (preset.atk_projectile)
			{
				add_projectile(preset.atk_projectile, node->g_pos + vec3(0.f, nav_agent->height * 0.5f, 0.f), target, [this](cCharacterPtr t) {
					if (t) 
						t->take_damage(atk);
				});
			}
			else
				target->take_damage(atk);

			action = ActionNone;
		}
		else
			attack_timer -= delta_time;
		nav_agent->set_speed_scale(0.f);
	}
}

void cCharacter::process_cast_to_target(cCharacterPtr target)
{

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
			exp_max = preset.exp;

		mov_sp = 100;
		atk_sp = 100;

		for (auto& ins : inventory)
		{
			if (ins)
			{
				auto& item = Item::get(ins->id);
				if (item.passive)
					item.passive(this);
			}
		}

		stats_dirty = false;
	}

	if (search_timer > 0)
		search_timer -= delta_time;
	if (attack_interval_timer > 0)
		attack_interval_timer -= delta_time;

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
