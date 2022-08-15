#include "character.h"
#include "item.h"
#include "projectile.h"

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

void cCharacter::set_target(cCharacterPtr character)
{
	if (target == character)
		return;
	if (target)
		target->entity->message_listeners.remove((uint)this);
	target = character;
	if (target)
	{
		target->entity->message_listeners.add([this](uint h, void*, void*) {
			if (h == "destroyed"_h)
				target = nullptr;
		}, (uint)this);
	}
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

void cCharacter::cmd_move_to(const vec3& pos)
{
	command = CommandMoveTo;
	move_location = pos;
}

void cCharacter::cmd_attack_target(cCharacterPtr character)
{
	command = CommandAttackTarget;
	set_target(character);
	action = ActionNone;
}

void cCharacter::cmd_attack_location(const vec3& pos)
{
	command = CommandAttackLocation;
	move_location = pos;
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

	auto move_to_traget = [this]() {
		nav_agent->set_target(move_location);
		action = ActionMove;

		if (length(nav_agent->desire_velocity()) <= 0.f)
		{
			nav_agent->stop();
			command = CommandIdle;
			action = ActionNone;
		}
		else
			action = ActionMove;
	};

	auto attack_target = [this]() {
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
	};

	switch (command)
	{
	case CommandIdle:
		break;
	case CommandMoveTo:
		move_to_traget();
		break;
	case CommandAttackTarget:
	{
		if (!target)
			command = CommandIdle;
		else
			attack_target();
	}
		break;
	case CommandAttackLocation:
	{
		auto dist = target ? distance(node->g_pos, target->node->g_pos) : 10000.f;
		if (dist > atk_distance + 12.f)
			set_target(nullptr);
		if (action != ActionAttack)
		{
			auto enemies = find_enemies(0.f, false, true);
			if (!enemies.empty() && dist > atk_distance)
				set_target(enemies.front());
		}

		if (target)
			attack_target();
		else
			move_to_traget();
	}
		break;
	}

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
