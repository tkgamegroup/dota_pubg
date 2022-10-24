#include "vision.h"
#include "character.h"
#include "network.h"

#include <flame/graphics/buffer.h>
#include <flame/graphics/image.h>
#include <flame/graphics/extension.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/renderer.h>

const uint resolution = 2;
const uint W = 256 * resolution;
const uint H = 256 * resolution;
static graphics::ImagePtr img_my_vision = nullptr;
static graphics::BufferPtr stagbuf = nullptr;

static std::map<uint, std::vector<uchar>> visions;

void init_vision()
{
	img_my_vision = graphics::Image::create(graphics::Format_R8_UNORM, uvec2(W, H), graphics::ImageUsageSampled | graphics::ImageUsageTransferDst);
	auto id = sRenderer::instance()->get_texture_res(img_my_vision->get_view());
	sRenderer::instance()->set_texture_res_name(id, "VISION");

	stagbuf = graphics::Buffer::create(W * H, graphics::BufferUsageTransferSrc, graphics::MemoryPropertyHost | graphics::MemoryPropertyCoherent);
	stagbuf->map();
}

bool get_vision(uint faction, const vec3& coord)
{
	auto it = visions.find(faction);
	if (it == visions.end())
		return false;
	auto buf = it->second.data();
	auto c = (ivec2)(coord.xz() * (float)resolution);
	if (c.x >= 0 && c.y >= 0 && c.x < W && c.y < W)
	{
		auto v = buf[c.y * W + c.x];
		return v > 0;
	}
	return false;
}

std::vector<ivec2> get_beam(int x, int y)
{
	std::vector<ivec2> ret;
	if (x == 0)
	{
		if (y > 0)
		{
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(0, +i));
		}
		else
		{
			y = -y;
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(0, -i));
		}
	}
	else if (y == 0)
	{
		if (x > 0)
		{
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(+i, 0));
		}
		else
		{
			x = -x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(-i, 0));
		}
	}
	else if (x > 0 && y > 0)
	{
		if (x == y)
		{
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(+i, +i));
		}
		else if (x > y)
		{
			auto k = y / (float)x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(+i, +i * k));
		}
		else
		{
			auto k = x / (float)y;
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(+i * k, +i));
		}
	}
	else if (x < 0 && y > 0)
	{
		if (-x == y)
		{
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(-i, +i));
		}
		else if (-x > y)
		{
			x = -x;
			auto k = y / (float)x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(-i, +i * k));
		}
		else
		{
			x = -x;
			auto k = x / (float)y;
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(-i * k, +i));
		}
	}
	else if (x > 0 && y < 0)
	{
		if (x == -y)
		{
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(+i, -i));
		}
		else if (x > -y)
		{
			y = -y;
			auto k = y / (float)x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(+i, -i * k));
		}
		else
		{
			y = -y;
			auto k = x / (float)y;
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(+i * k, -i));
		}
	}
	else if (x < 0 && y < 0)
	{
		if (x == y)
		{
			x = -x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(-i, -i));
		}
		else if (-x > -y)
		{
			x = -x;
			y = -y;
			auto k = y / (float)x;
			for (auto i = 0; i < x; i++)
				ret.push_back(ivec2(-i, -i * k));
		}
		else
		{
			x = -x;
			y = -y;
			auto k = x / (float)y;
			for (auto i = 0; i < y; i++)
				ret.push_back(ivec2(-i * k, -i));
		}
	}
	return ret;
}

void update_vision()
{
	for (auto& f : characters_by_faction)
	{
		if (main_player.faction != f.first && multi_player != SinglePlayer && multi_player != MultiPlayerAsHost)
			continue;

		auto it = visions.find(f.first);
		if (it == visions.end())
		{
			it = visions.emplace(f.first, std::vector<uchar>()).first;
			it->second.resize(W * H);
			if (f.first == FactionCreep)
				memset(it->second.data(), 255, W * H);
		}

		auto buf = it->second.data();
		if (f.first != FactionCreep) // well, creeps have full vision
		{
			auto changed = false;
			for (auto character : f.second)
			{
				auto pos = character->node->pos;
				auto coord = (ivec2)(pos.xz() * (float)resolution);
				if (coord != character->vision_coord)
				{
					changed = true;
					character->vision_coord = coord;
				}
			}

			if (changed)
			{
				memset(buf, 0, W * H);

				for (auto character : f.second)
				{

					auto pos = character->node->pos;
					auto coord = character->vision_coord;
					auto height = pos.y;
					auto range = character->vision_range;
					auto elev = main_terrain.terrain->extent.y / 4.f;
					auto level = 0;
					for (; level < 4; level++)
					{
						if (height < elev)
							break;
						height -= elev;
					}
					height = (level + 1) * elev;

					// add blockers (trees, rocks)
					std::vector<cNodePtr> objs;
					sScene::instance()->octree->get_colliding(pos, range / (float)resolution, objs);
					std::vector<std::pair<vec2, float>> blockers;
					for (auto n : objs)
					{
						if (n->entity->name == "blocker")
							blockers.emplace_back(n->g_pos.xz(), n->g_scl.x * 0.5f);
					}

					for (auto y = int(coord.y - range); y <= coord.y + range; y++)
					{
						for (auto x = int(coord.x - range); x <= coord.x + range; x++)
						{
							if (x >= 0 && y >= 0 && x < W && y < H)
							{
								auto mark = false;
								if (main_terrain.get_coord(vec2((x + 0.5f) / W, (y + 0.5f) / H)).y > height) // high elevation
									mark = true;
								else
								{
									for (auto& b : blockers)
									{
										if (distance(vec2(x, y) / (float)resolution, b.first) > b.second)
											continue;
										if (distance(vec2(x + 1, y) / (float)resolution, b.first) > b.second)
											continue;
										if (distance(vec2(x + 1, y + 1) / (float)resolution, b.first) > b.second)
											continue;
										if (distance(vec2(x, y + 1) / (float)resolution, b.first) > b.second)
											continue;
										mark = true;
										break;
									}
								}
								if (mark)
									buf[y * W + x] = 1;
							}
						}
					}

					auto cast_beam = [&](const std::vector<ivec2>& b) {
						for (auto& p : b)
						{
							if (length((vec2)p) > range)
								break;
							auto c = coord + p;
							if (c.x < 0 || c.y < 0 || c.x >= W || c.y >= H)
								break;
							auto& dst = buf[c.y * W + c.x];
							if (dst == 1)
								break;
							dst = 255;
						}
					};
					cast_beam(get_beam(+range, 0));
					cast_beam(get_beam(-range, 0));
					cast_beam(get_beam(0, +range));
					cast_beam(get_beam(0, -range));

					cast_beam(get_beam(+range, +range));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(+range, +i));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(+i, +range));

					cast_beam(get_beam(-range, +range));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(-range, +i));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(-i, +range));

					cast_beam(get_beam(+range, -range));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(+range, -i));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(+i, -range));

					cast_beam(get_beam(-range, -range));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(-range, -i));
					for (auto i = 1; i < range; i++)
						cast_beam(get_beam(-i, -range));
				}

				if (f.first == main_player.faction)
				{
					graphics::Queue::get()->wait_idle();

					graphics::InstanceCommandBuffer cb;
					memcpy(stagbuf->mapped, visions[main_player.faction].data(), stagbuf->size);
					cb->image_barrier(img_my_vision, {}, graphics::ImageLayoutTransferDst);
					cb->copy_buffer_to_image(stagbuf, img_my_vision, graphics::BufferImageCopy(uvec2(W, H)));
					cb->image_barrier(img_my_vision, {}, graphics::ImageLayoutShaderReadOnly);
					cb.excute();
				}
			}
		}
	}

	// update character's visiable
	characters_in_vision.clear();
	for (auto& pair : characters_by_id)
	{
		auto v = get_vision(main_player.faction, pair.second->node->pos);
		pair.second->entity->children[0]->set_enable(v);
		if (v)
			characters_in_vision.push_back(pair.second);
	}
}
