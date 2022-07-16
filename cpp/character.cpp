#include "character.h"

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
				ImGui::Begin(str(this).c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMouseInputs);
				//ImGui::Text("%s %d %d", entity->name.c_str(), state, action);
				ImGui::Dummy(ImVec2(w, h));
				auto dl = ImGui::GetWindowDrawList();
				auto p0 = (vec2)ImGui::GetItemRectMin();
				auto p1 = (vec2)ImGui::GetItemRectMax();
				dl->AddRectFilled(p0, p0 + vec2((float)hp / (float)hp_max * w, h), ImColor(0.f, 1.f, 0.f));
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

std::vector<cCharacterPtr> cCharacter::find_enemies(float radius)
{
	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(node->g_pos, radius ? radius : 3.5f, objs, CharacterTag);
	std::vector<cCharacterPtr> enemies;
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && chr->faction != faction)
			enemies.push_back(chr);
	}
	return enemies;
}

void cCharacter::change_target(cCharacterPtr character)
{
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

void cCharacter::enter_move_state(const vec3& pos)
{
	state = StateMove;
	action = ActionNone;
	move_target = pos;
	nav_agent->set_target(pos);
}

void cCharacter::enter_move_attack_state(const vec3& pos)
{
	state = StateMoveAttack;
	action = ActionNone;
	move_target = pos;
	nav_agent->set_target(pos);
}

void cCharacter::enter_battle_state(cCharacterPtr target)
{
	state = StateBattle;
	action = ActionNone;
	change_target(target);
}

void cCharacter::die()
{
	if (dead)
		return;
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

	auto dt = delta_time;
	if (search_timer > 0)
		search_timer -= dt;
	if (attack_interval_timer > 0)
		attack_interval_timer -= dt;
	if (chase_timer > 0)
		chase_timer -= dt;

	auto search_enemies_and_enter_battle = [this]() {
		if (search_timer <= 0.f)
		{
			search_timer = 0.3f + random01() * 0.1f;
			auto enemies = find_enemies();
			if (!enemies.empty())
			{
				enter_battle_state(enemies.front());
				return true;
			}
		}
		return false;
	};

	switch (state)
	{
	case StateIdle:
		if (next_state != StateIdle)
		{
			switch (next_state)
			{
			case StateMoveAttack:
				enter_move_attack_state(move_target);
				break;
			}
			next_state = StateIdle;
		}
		else
		{
			if (ai_id == 1)
				search_enemies_and_enter_battle();
		}
		break;
	case StateMoveAttack:
		if (search_enemies_and_enter_battle())
		{
			next_state = StateMoveAttack;
			break;
		}
	case StateMove:
		if (length(nav_agent->desire_velocity()) <= 0.f)
		{
			nav_agent->stop();
			state = StateIdle;
			action = ActionNone;
			if (armature)
				armature->play("idle"_h, 0.3f);
		}
		else
		{
			action = ActionMove;
			if (armature)
				armature->play("run"_h);
		}
		break;
	case StateBattle:
	{
		if (action == ActionNone)
		{
			if (ai_id != 0)
			{
				auto enemies = find_enemies();
				std::vector<std::pair<float, cCharacterPtr>> enemies_with_dist(enemies.size());
				auto self_pos = node->g_pos;
				for (auto i = 0; i < enemies.size(); i++)
				{
					auto c = enemies[i];
					enemies_with_dist[i] = std::make_pair(distance(c->node->g_pos, self_pos), c);
				}
				std::sort(enemies_with_dist.begin(), enemies_with_dist.end(), [](const auto& a, const auto& b) {
					return a.first < b.first;
					});
				auto tar_dist = target ? distance(target->node->g_pos, self_pos) : 100000.f;
				if (tar_dist > 8.f)
					change_target(nullptr);
				if (!target || tar_dist > atk_distance)
				{
					if (!enemies_with_dist.empty())
						change_target(enemies_with_dist.front().second);
					else
						change_target(nullptr);
				}
			}
		}

		if (!target)
		{
			state = StateIdle;
			action = ActionNone;
			if (armature)
				armature->play("idle"_h, 0.3f);
		}
		else
		{
			auto tar_pos = target->node->g_pos;
			auto self_pos = node->g_pos;
			auto dir = tar_pos - self_pos;
			auto dist = length(dir);
			dir = normalize(dir);
			auto ang_diff = abs(angle_diff(node->get_eul().x, degrees(atan2(dir.x, dir.z))));

			if (action == ActionNone || action == ActionMove)
			{
				if (ang_diff <= 60.f && dist <= atk_distance)
				{
					if (attack_interval_timer <= 0.f)
					{
						action = ActionAttack;
						if (armature)
							armature->play("attack"_h);
						nav_agent->stop();
						attack_interval_timer = atk_interval;
						attack_timer = atk_interval * atk_precast;
					}
				}
				else
				{
					action = ActionMove;
					if (armature)
						armature->play("run"_h);
					if (chase_timer <= 0.f)
					{
						nav_agent->set_target(tar_pos);
						chase_timer = 0.1f + random01() * 0.05f;
					}
				}
			}
			else if (action == ActionAttack)
			{
				nav_agent->set_target(tar_pos, true);
				if (attack_timer <= 0.f)
				{
					if (target->hp > atk)
						target->hp -= atk;
					else
					{
						target->hp = 0;
						target->dead = true;
					}

					action = ActionNone;
					if (armature)
						armature->play("idle"_h, 0.5f);
				}
				else
					attack_timer -= dt;
			}
		}
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
