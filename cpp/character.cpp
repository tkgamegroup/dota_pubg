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
	gui_lis = graphics::gui_callbacks.add([this]() {
		auto& tars = sRenderer::instance()->iv_tars;
		if (!tars.empty() && main_camera.camera)
		{
			auto tar = tars.front();
			auto p = main_camera.camera->proj_view_mat * vec4(node->g_pos + vec3(0.f, height + 0.5f, 0.f), 1.f);
			p /= p.w;
			p.xy = (p.xy * 0.5f + 0.5f) * vec2(tar->image->size);
			p.xy += sInput::instance()->offset;
			ImGui::SetNextWindowPos(p.xy(), ImGuiCond_Always, ImVec2(0.5f, 1.f));
			ImGui::Begin(str(this).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
			ImGui::TextUnformatted(entity->name.c_str());
			ImGui::Text("HP: %d", hp);
			ImGui::Text("State: %d", state);
			ImGui::Text("Action: %d", action);
			ImGui::End();
		}
	});
}

void cCharacter::on_inactive()
{
	graphics::gui_callbacks.remove(gui_lis);
}

std::vector<cCharacterPtr> cCharacter::find_enemies(float radius)
{
	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(node->g_pos, radius ? radius : 5.f, objs, CharacterTag);
	std::vector<cCharacterPtr> enemies;
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && chr->faction != faction)
			enemies.push_back(chr);
	}
	return enemies;
}

void cCharacter::enter_move_state(const vec3& pos)
{
	state = StateMove;
	action = ActionNone;
	nav_agent->set_target(pos);
}

void cCharacter::enter_battle_state(const std::vector<cCharacterPtr>& enemies)
{
	state = StateBattle;
	action = ActionNone;
	hate_list = enemies;
	for (auto chr : enemies)
	{
		auto e = chr->entity;
		e->message_listeners.add([this, e](uint h, void*, void*) {
			if (h == "destroyed"_h)
			{
				for (auto& chr : hate_list)
				{
					if (chr->entity == e)
						chr = nullptr;
				}
			}
		});
	}
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
	auto dt = delta_time;
	if (search_timer > 0)
		search_timer -= dt;
	if (attack_interval_timer > 0)
		attack_interval_timer -= dt;
	if (chase_timer > 0)
		chase_timer -= dt;

	switch (state)
	{
	case StateIdle:
		if (ai_id == 1)
		{
			if (search_timer <= 0.f)
			{
				auto enemies = find_enemies();
				if (!enemies.empty())
					enter_battle_state(enemies);
				search_timer = 0.3f + random01() * 0.1f;
			}
		}
		break;
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
		for (auto it = hate_list.begin(); it != hate_list.end(); )
		{
			if (*it == nullptr)
				it = hate_list.erase(it);
			else
				it++;
		}
		if (hate_list.empty())
		{
			state = StateIdle;
			action = ActionNone;
			if (armature)
				armature->play("idle"_h, 0.3f);
		}
		else
		{
			if (action == ActionNone)
				target = hate_list[0];

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
