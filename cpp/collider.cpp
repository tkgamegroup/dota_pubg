#include "collider.h"
#include "character.h"

#include <flame/graphics/gui.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

void cSectorCollider::on_active()
{
	off = 0.f;
	r0 = r1 = 0.f;
	t = 0.f;
	rnd = rand();

	if (in_editor)
	{
		node->drawers.add([this](DrawData& draw_data) {
			switch (draw_data.pass)
			{
			case PassPrimitive:
				if (t >= collide_time.x && t <= collide_time.y)
				{
					auto& mat = node->transform;

					auto circle_lod = [](float r) {
						return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
					};
					auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r1));
					std::vector<vec3> pts;
					auto i_beg = circle_draw.get_idx(direction_angle - central_angle);
					auto i_end = circle_draw.get_idx(direction_angle + central_angle);
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

					draw_data.primitives.emplace_back("LineList"_h, std::move(pts), cvec4(0, 255, 0, 255));
				}
				break;
			}
		}, "collider"_h);
	}
}

void cSectorCollider::on_inactive()
{
	if (in_editor)
		node->drawers.remove("collider"_h);
}

void cSectorCollider::update()
{
	off += speed * delta_time;
	r0 = start_radius + max(off - window_length, 0.f);
	r1 = radius + off;
	c = dir_xz(direction_angle) * -start_radius;
	t += delta_time;

	if (t >= collide_time.x && t <= collide_time.y)
	{
		auto pos = c + node->g_pos;
		for (auto& character : find_characters(faction, pos, r1, r0, central_angle, direction_angle))
		{
			static ParameterPack parameters;
			if (character->add_marker(rnd, collide_time.y))
				callback.execute(host, character, vec3(0.f), parameters, 0);
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
