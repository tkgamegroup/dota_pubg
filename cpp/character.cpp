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
	character->move_to(location);
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
		character->attack_target(target.obj);
}

CommandAttackLocation::CommandAttackLocation(cCharacterPtr character, const vec3& _location) :
	Command(character)
{
	location = _location;
}

void CommandAttackLocation::update()
{
	auto dist = target.obj ? distance(character->node->g_pos, target.obj->node->g_pos) : 10000.f;
	if (dist > character->atk_distance + 12.f)
		target.set(nullptr);
	if (character->action != ActionAttack)
	{
		auto enemies = character->find_enemies(0.f, false, true);
		if (!enemies.empty() && dist > character->atk_distance)
			target.set(enemies.front());
	}

	if (target.obj)
		character->attack_target(target.obj);
	else
		character->move_to(location);
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
		auto self_pos = character->node->g_pos;
		auto tar_pos = target.obj->node->g_pos;
		if (distance(self_pos, tar_pos) <= 1.f)
		{
			pick_up_chest(character, target.obj);

			character->nav_agent->stop();
			new CommandIdle(character);
		}
		else
		{
			character->nav_agent->set_target(tar_pos);
			character->action = ActionMove;
		}
	}
}

cCharacter::~cCharacter()
{
	node->measurers.remove("character"_h);

	graphics::gui_callbacks.remove((uint)this);
}

void cCharacter::set_atk_projectile_name(const std::filesystem::path& name)
{
	if (atk_projectile_name == name)
		return;
	atk_projectile_name = name;

	EntityPtr _atk_projectile = nullptr;
	if (!atk_projectile_name.empty())
	{
		_atk_projectile = Entity::create();
		if (!_atk_projectile->load(atk_projectile_name))
		{
			delete _atk_projectile;
			_atk_projectile = nullptr;
			return;
		}
	}

	delete atk_projectile;
	atk_projectile = _atk_projectile;
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

uint gain_exp_from_killing(uint lv)
{
	return (100.f * (1.f - pow(NextLvExpFactor, lv)) / (1.f - NextLvExpFactor)) * 0.13f;
}

void cCharacter::inflict_damage(cCharacterPtr target, uint value)
{
	if (target->take_damage(value))
	{
		if (exp_max > 0)
		{
			exp += gain_exp_from_killing(target->lv);
			while (exp > exp_max)
			{
				exp -= exp_max;
				level_up();
			}
		}
	}
}

bool cCharacter::take_damage(uint value)
{
	if (hp > value)
	{
		hp -= value;
		return false;
	}

	die();
	return true;
}

void cCharacter::manipulate_item(int idx0, int idx1, int item_id)
{

}

void cCharacter::level_up()
{
	lv++;
	exp_max *= NextLvExpFactor;

	atk++;
	hp_max += 10;
	hp += 10;
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

void cCharacter::move_to(const vec3& target)
{
	nav_agent->set_target(target);

	if (length(nav_agent->desire_velocity()) <= 0.f)
	{
		nav_agent->stop();
		new CommandIdle(this);
	}
	else
		action = ActionMove;
}

void cCharacter::attack_target(cCharacterPtr target)
{
	auto self_pos = node->g_pos;
	auto tar_pos = target->node->g_pos;
	auto dir = tar_pos - self_pos;
	auto dist = length(dir);
	dir = normalize(dir);
	auto ang_diff = abs(angle_diff(node->get_eul().x, degrees(atan2(dir.x, dir.z))));

	if (action == ActionNone || action == ActionMove)
	{
		if (dist <= atk_distance)
		{
			if (ang_diff <= 60.f && attack_interval_timer <= 0.f)
			{
				action = ActionAttack;
				attack_speed = max(0.01f, atk_sp / 100.f);
				attack_interval_timer = atk_interval / attack_speed;
				attack_timer = attack_interval_timer * atk_precast;
			}
			else
				action = ActionNone;
			nav_agent->set_target(tar_pos, true);
		}
		else
		{
			action = ActionMove;
			nav_agent->set_target(tar_pos);
		}
	}
	else if (action == ActionAttack)
	{
		if (attack_timer <= 0.f)
		{
			if (atk_projectile)
			{
				auto e = atk_projectile->copy();
				e->get_component_t<cNode>()->set_pos(node->g_pos + vec3(0.f, nav_agent->height * 0.5f, 0.f));
				e->get_component_t<cProjectile>()->setup(target, [this](cCharacterPtr t) {
					if (t)
						t->take_damage(atk);
				});
				root->add_child(e);
			}
			else
				target->take_damage(atk);

			action = ActionNone;
		}
		else
			attack_timer -= delta_time;
		nav_agent->set_target(tar_pos, true);
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
		mov_sp = 100;
		atk_sp = 100;

		for (auto& id : inventory)
		{
			if (id != -1)
			{
				auto& item = Item::get(id);
				mov_sp += item.add_mov_sp;
				atk_sp += item.add_atk_sp;
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
			move_speed = max(0.1f, mov_sp / 100.f);
			armature->playing_speed = move_speed;
			armature->play("run"_h);
			nav_agent->set_speed_scale(move_speed);
			break;
		case ActionAttack:
			armature->playing_speed = attack_speed;
			armature->play("attack"_h);
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
