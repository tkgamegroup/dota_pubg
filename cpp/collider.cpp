#include "collider.h"

#include <flame/graphics/gui.h>
#include <flame/universe/draw_data.h>
#include <flame/universe/components/node.h>

void cSectorCollider::on_active()
{
	if (in_editor)
	{
		node->drawers.add([this](DrawData& draw_data) {
			switch (draw_data.pass)
			{
			case PassPrimitive:
			{
				auto& mat = node->transform;

				auto circle_lod = [](float r) {
					return r > 8.f ? 3 : (r > 4.f ? 3 : (r > 2.f ? 2 : (r > 1.f ? 1 : 0)));
				};
				auto r0 = start_radius + (off > length ? off - length : 0.f);
				auto r1 = radius + off;
				auto circle_draw = graphics::GuiCircleDrawer(circle_lod(r1));
				auto center = dir_xz(dir) * -start_radius;
				std::vector<vec3> pts;
				auto i_beg = circle_draw.get_idx(dir - angle);
				auto i_end = circle_draw.get_idx(dir + angle);
				for (auto i = i_beg; i < i_end; i++)
				{
					auto a = mat * vec4(center + vec3(r0 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f);
					auto b = mat * vec4(center + vec3(r0 * circle_draw.get_pt(i), 0.f).xzy(), 1.f);
					auto c = mat * vec4(center + vec3(r1 * circle_draw.get_pt(i), 0.f).xzy(), 1.f);
					auto d = mat * vec4(center + vec3(r1 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f);

					pts.push_back(mat * vec4(center + vec3(r0 * circle_draw.get_pt(i), 0.f).xzy(), 1.f));
					pts.push_back(mat * vec4(center + vec3(r0 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f));
					pts.push_back(mat * vec4(center + vec3(r1 * circle_draw.get_pt(i), 0.f).xzy(), 1.f));
					pts.push_back(mat * vec4(center + vec3(r1 * circle_draw.get_pt(i + 1), 0.f).xzy(), 1.f));
				}

				pts.push_back(mat * vec4(center + vec3(r0 * circle_draw.get_pt(i_beg), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(center + vec3(r1 * circle_draw.get_pt(i_beg), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(center + vec3(r0 * circle_draw.get_pt(i_end), 0.f).xzy(), 1.f));
				pts.push_back(mat * vec4(center + vec3(r1 * circle_draw.get_pt(i_end), 0.f).xzy(), 1.f));

				draw_data.primitives.emplace_back("LineList"_h, std::move(pts), cvec4(0, 255, 0, 255));
			}
				break;
			}
		}, "collider"_h);
	}
}

void cSectorCollider::on_inactive()
{
	off = 0.f;

	if (in_editor)
		node->drawers.remove("collider"_h);
}

void cSectorCollider::start()
{

}

void cSectorCollider::update()
{
	off += speed * delta_time;


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
