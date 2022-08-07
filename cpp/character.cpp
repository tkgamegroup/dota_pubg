#include "character.h"
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
		*ret = AABB(AABB(vec3(-radius, 0.f, -radius), vec3(radius, height, radius)).get_points(node->transform));
		return true;
	}, "character"_h);
}

void cCharacter::on_active()
{
	graphics::gui_callbacks.add([this]() {
		auto& tars = sRenderer::instance()->iv_tars;
		if (!tars.empty() && main_camera.camera)
		{
			auto tar = tars.front();
			auto p = main_camera.camera->proj_view_mat * vec4(node->g_pos + vec3(0.f, height + 0.1f, 0.f), 1.f);
			p /= p.w;
			if (p.x > -1.f && p.x < 1.f && p.y > -1.f && p.y < 1.f && p.z > 0.f && p.z < 1.f)
			{
				p.xy = (p.xy * 0.5f + 0.5f) * vec2(tar->image->size);
				p.xy += sInput::instance()->offset;
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
				ImGui::SetNextWindowPos(p.xy(), ImGuiCond_Always, ImVec2(0.5f, 1.f));
				const auto w = 80.f;
				const auto h = 5.f;
				ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
				ImGui::Begin(str(this).c_str(), nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMouseInputs);
				//ImGui::Text("%s %d %d", entity->name.c_str(), state, action);
				ImGui::Dummy(ImVec2(w, h));
				auto dl = ImGui::GetWindowDrawList();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)hp / (float)hp_max * w, h), 
					faction == main_player.character->faction ? ImColor(0.f, 1.f, 0.f) : ImColor(1.f, 0.f, 0.f));
				dl->AddRect(p0, p0 + vec2(100.f, 5.f), ImColor(1.f, 1.f, 1.f));
				ImGui::End();
				ImGui::PopStyleVar(2);
			}
		}
	}, (uint)this);
}

void cCharacter::on_inactive()
{
	graphics::gui_callbacks.remove((uint)this);
}

std::vector<cCharacterPtr> cCharacter::find_enemies(float radius, bool ignore_timer, bool sort)
{
	std::vector<cCharacterPtr> ret;
	if (!ignore_timer)
	{
		if (search_timer <= 0.f)
			search_timer = 0.1f + random01() * 0.05f;
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

void cCharacter::die()
{
	if (dead)
		return;
}

void cCharacter::cmd_move_to(const vec3& pos)
{
	command = CommandMoveTo;
	move_target = pos;
}

void cCharacter::cmd_attack_target(cCharacterPtr character)
{
	command = CommandAttackTarget;
	set_target(character);
	action = ActionNone;
	if (armature)
		armature->play("idle"_h);
}

void cCharacter::cmd_move_attack(const vec3& pos)
{
	command = CommandMoveAttack;
	move_target = pos;
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
		armature->play("idle"_h);
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

	if (search_timer > 0)
		search_timer -= delta_time;
	if (attack_interval_timer > 0)
		attack_interval_timer -= delta_time;
	if (chase_timer > 0)
		chase_timer -= delta_time;

	auto move_to_traget = [this]() {
		nav_agent->set_target(move_target);
		action = ActionMove;
		if (armature)
			armature->play("run"_h);

		if (length(nav_agent->desire_velocity()) <= 0.f)
		{
			nav_agent->stop();
			command = CommandIdle;
			action = ActionNone;
			if (armature)
				armature->play("idle"_h);
		}
		else
		{
			action = ActionMove;
			if (armature)
				armature->play("run"_h);
		}
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
					if (armature)
						armature->play("attack"_h);
					attack_interval_timer = atk_interval;
					attack_timer = atk_interval * atk_precast;
				}
				else
				{
					action = ActionNone;
					if (armature)
						armature->play("idle"_h);
				}
				nav_agent->set_target(tar_pos, true);
			}
			else
			{
				action = ActionMove;
				if (armature)
					armature->play("run"_h);
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
					e->get_component_t<cNode>()->set_pos(node->g_pos + vec3(0.f, height, 0.f));
					e->get_component_t<cProjectile>()->set_target(target);
					root->add_child(e);
				}
				else
				{
					if (target->hp > atk)
						target->hp -= atk;
					else
					{
						target->hp = 0;
						target->dead = true;
					}
				}

				action = ActionNone;
				if (armature)
					armature->play("idle"_h);
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
	case CommandMoveAttack:
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
