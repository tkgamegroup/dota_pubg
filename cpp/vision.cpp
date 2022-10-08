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

const uint vision_map_w = 256;
const uint vision_map_h = 256;
static graphics::ImagePtr img_my_vision = nullptr;
static graphics::BufferPtr stagbuf = nullptr;

static std::map<uint, std::vector<uchar>> visions;

bool get_vision(uint faction, const vec3& coord)
{
	auto it = visions.find(faction);
	if (it == visions.end())
		return false;
	auto buf = it->second.data();
	auto c = (ivec2)coord.xz();
	if (c.x >= 0 && c.y >= 0 && c.x < vision_map_w && c.y < vision_map_w)
	{
		auto v = buf[c.y * vision_map_w + c.x];
		return v > 0;
	}
	return false;
}

void update_vision()
{
	auto w = vision_map_w; auto h = vision_map_h;
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		for (auto& f : characters_by_faction)
		{
			auto it = visions.find(f.first);
			if (it == visions.end())
			{
				it = visions.emplace(f.first, std::vector<uchar>()).first;
				it->second.resize(w * h);
			}
			auto buf = it->second.data();

			if (f.first == FactionCreep) // well, creeps have full vision
			{
				memset(buf, 255, w * h);
			}
			else
			{
				for (auto character : f.second)
				{
					auto pos = character->node->pos;
					auto coord = (ivec2)pos.xz();
					if (coord != character->vision_coord)
					{
						struct Beam
						{
							std::vector<ivec2> pts;
						};
						auto get_beam = [&](int x, int y)->Beam {
							Beam b;
							if (x == 0)
							{
								if (y > 0)
								{
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(0, +i));
								}
								else
								{
									y = -y;
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(0, -i));
								}
							}
							else if (y == 0)
							{
								if (x > 0)
								{
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(+i, 0));
								}
								else
								{
									x = -x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(-i, 0));
								}
							}
							else if (x > 0 && y > 0)
							{
								if (x == y)
								{
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(+i, +i));
								}
								else if (x > y)
								{
									auto k = y / (float)x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(+i, +i * k));
								}
								else
								{
									auto k = x / (float)y;
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(+i * k, +i));
								}
							}
							else if (x < 0 && y > 0)
							{
								if (-x == y)
								{
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(-i, +i));
								}
								else if (-x > y)
								{
									x = -x;
									auto k = y / (float)x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(-i, +i * k));
								}
								else
								{
									x = -x;
									auto k = x / (float)y;
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(-i * k, +i));
								}
							}
							else if (x > 0 && y < 0)
							{
								if (x == -y)
								{
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(+i, -i));
								}
								else if (x > -y)
								{
									y = -y;
									auto k = y / (float)x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(+i, -i * k));
								}
								else
								{
									y = -y;
									auto k = x / (float)y;
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(+i * k, -i));
								}
							}
							else if (x < 0 && y < 0)
							{
								if (x == y)
								{
									x = -x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(-i, -i));
								}
								else if (-x > -y)
								{
									x = -x;
									y = -y;
									auto k = y / (float)x;
									for (auto i = 0; i < x; i++)
										b.pts.push_back(ivec2(-i, -i * k));
								}
								else
								{
									x = -x;
									y = -y;
									auto k = x / (float)y;
									for (auto i = 0; i < y; i++)
										b.pts.push_back(ivec2(-i * k, -i));
								}
							}
							return b;
						};

						memset(buf, 0, w * h);

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
						sScene::instance()->octree->get_colliding(pos, range, objs);
						for (auto n : objs)
						{
							if (n->entity->name == "blocker")
							{
								auto pos = n->g_pos;
								auto coord = (ivec2)pos.xz();
								buf[coord.y * w + coord.x] = 1;
								if (pos.x < 0.5f)
								{
									if (coord.x > 0)
										buf[coord.y * w + (coord.x - 1)] = 1;
								}
								else
								{
									if (coord.x < w - 1)
										buf[coord.y * w + (coord.x + 1)] = 1;
								}
								if (pos.y < 0.5f)
								{
									if (coord.y > 0)
										buf[(coord.y - 1) * w + coord.x] = 1;
								}
								else
								{
									if (coord.y < h - 1)
										buf[(coord.y + 1) * w + coord.x] = 1;
								}
							}
						}
						// add blockers (high elevation)
						for (auto y = int(coord.y - range); y <= coord.y + range; y++)
						{
							for (auto x = int(coord.x - range); x <= coord.x + range; x++)
							{
								if (x >= 0 && y >= 0 && x < w && y < h)
								{
									if (main_terrain.get_coord(vec2((x + 0.5f) / w, (y + 0.5f) / h)).y > height)
										buf[y * w + x] = 1;
								}
							}
						}

						auto cast_beam = [&](const Beam& b) {
							for (auto& p : b.pts)
							{
								if (length((vec2)p) > range)
									break;
								auto c = coord + p;
								if (c.x < 0 || c.y < 0 || c.x >= w || c.y >= w)
									break;
								auto& dst = buf[c.y * w + c.x];
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

						character->vision_coord = coord;

						if (f.first == main_player.character->faction)
						{
							graphics::Queue::get()->wait_idle();

							if (!img_my_vision)
							{
								img_my_vision = graphics::Image::create(graphics::Format_R8_UNORM, uvec2(w, h), graphics::ImageUsageSampled | graphics::ImageUsageTransferDst);
								auto id = sRenderer::instance()->get_texture_res(img_my_vision->get_view());
								sRenderer::instance()->set_texture_res_name(id, "VISION");
							}
							if (!stagbuf)
							{
								stagbuf = graphics::Buffer::create(vision_map_w * vision_map_h, graphics::BufferUsageTransferSrc, graphics::MemoryPropertyHost | graphics::MemoryPropertyCoherent);
								stagbuf->map();
							}

							graphics::InstanceCommandBuffer cb;
							memcpy(stagbuf->mapped, visions[main_player.character->faction].data(), stagbuf->size);
							cb->copy_buffer_to_image(stagbuf, img_my_vision, graphics::BufferImageCopy(uvec2(w, h)));
							cb.excute();
						}
					}
				}
			}
		}
	}

	// update character's visiable
	for (auto& pair : characters_by_guid)
		pair.second->entity->children[0]->set_enable(get_vision(main_player.character->faction, pair.second->node->pos));
}
