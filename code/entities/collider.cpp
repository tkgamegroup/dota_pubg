#include <flame/graphics/gui.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/renderer.h>

#include "../game.h"
#include "collider.h"
#include "character.h"

void process_colliding(const std::vector<cCharacterPtr>& characters, std::vector<Tracker>& last_collidings, const Listeners<void(cCharacterPtr character, uint type)>& callbacks)
{
	for (auto c : characters)
	{
		auto found = false;
		for (auto& _c : last_collidings)
		{
			if (c == _c.get<cCharacterPtr>())
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			callbacks.call(c, "enter"_h);
			last_collidings.emplace_back(c);
		}
	}
	for (auto it = last_collidings.begin(); it != last_collidings.end();)
	{
		auto found = false;
		for (auto _c : characters)
		{
			if (it->get<cCharacterPtr>() == _c)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			callbacks.call(it->get<cCharacterPtr>(), "exit"_h);
			it = last_collidings.erase(it);
		}
		else
			it++;
	}
}

static int circle_lod(float r) 
{
	return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
};

void cCircleCollider::update()
{
	auto characters = faction != FactionNone ? find_characters_within_circle(faction, node->global_pos(), radius) : std::vector<cCharacterPtr>();
	process_colliding(characters, last_collidings, callbacks);

	if (enable_collider_debugging)
	{
		auto& mat = node->transform;

		auto circle_draw = graphics::GuiCircleDrawer(circle_lod(radius));
		std::vector<vec3> pts;
		for (auto i = 0; i < circle_draw.pts.size(); i++)
		{
			pts.push_back(mat * vec4(vec3(radius * circle_draw.get_pt(i), 0.f).xzy(), 1.f));
			pts.push_back(mat * vec4(vec3(radius * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f));
		}

		sRenderer::instance()->draw_primitives("LineList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 255));
	}
}

struct cCircleColliderCreate : cCircleCollider::Create
{
	cCircleColliderPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cCircleCollider;
	}
}cCircleCollider_create;
cCircleCollider::Create& cCircleCollider::create = cCircleCollider_create;

void cSectorCollider::on_active()
{
	off = 0.f;
	r0 = r1 = 0.f;
	t = 0.f;
}

void cSectorCollider::on_inactive()
{
}

void cSectorCollider::update()
{
	off += speed * delta_time;
	r0 = inner_radius + max(off - length, 0.f);
	r1 = outer_radius + off;
	yaw_angle = yaw(node->g_qut);
	t += delta_time;

	if (t >= delay && t <= delay + duration)
	{
		auto pos = node->transform * vec4(-inner_radius, 0.f, 0.f, 1.f);
		auto characters = find_characters_within_sector(faction, pos, r0, r1, angle, yaw_angle);
		process_colliding(characters, last_collidings, callbacks);

		if (enable_collider_debugging)
		{
			auto& mat = node->transform;
			auto c = vec3(-inner_radius, 0.f, 0.f);

			auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r1));
			std::vector<vec3> pts;
			auto i_beg = circle_draw.get_idx(yaw_angle - angle);
			auto i_end = circle_draw.get_idx(yaw_angle + angle);
			for (auto i = i_beg; i < i_end; i++)
			{
				pts.push_back(mat * vec4(c + vec3(r0 * circle_draw.get_pt(i), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(c + vec3(r0 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(c + vec3(r1 * circle_draw.get_pt(i), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(c + vec3(r1 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f));
			}

			pts.push_back(mat * vec4(c + vec3(r0 * circle_draw.get_pt(i_beg), 0.f).xzy(), 1.f));
			pts.push_back(mat * vec4(c + vec3(r1 * circle_draw.get_pt(i_beg), 0.f).xzy(), 1.f));
			pts.push_back(mat * vec4(c + vec3(r0 * circle_draw.get_pt(i_end), 0.f).xzy(), 1.f));
			pts.push_back(mat * vec4(c + vec3(r1 * circle_draw.get_pt(i_end), 0.f).xzy(), 1.f));

			sRenderer::instance()->draw_primitives("LineList"_h, pts.data(), pts.size(), cvec4(0, 255, 0, 255));
		}
	}
}

struct cSectorColliderCreate : cSectorCollider::Create
{
	cSectorColliderPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cSectorCollider;
	}
}cSectorCollider_create;
cSectorCollider::Create& cSectorCollider::create = cSectorCollider_create;

bool enable_collider_debugging = false;
