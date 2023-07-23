#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/buffer.h>
#include <flame/graphics/image.h>
#include <flame/graphics/extension.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/mesh.h>
#include <flame/universe/components/terrain.h>
#include <flame/universe/components/volume.h>
#include <flame/universe/systems/renderer.h>
#include <flame/universe/systems/scene.h>

#include "game.h"
#include "player.h"
#include "map.h"
#include "entities/object.h"
#include "entities/character.h"

cNodePtr map_node = nullptr;
cTerrainPtr hf_terrain = nullptr; // height field terrain
cVolumePtr mc_terrain = nullptr; // marching cubes terrain
vec3 map_extent;

std::vector<vec3> site_positions;
std::vector<std::pair<float, int>> site_centrality;

void init_map(EntityPtr e, uint type)
{
	map_node = e->get_component<cNode>();

	hf_terrain = e->get_component<cTerrain>();
	mc_terrain = e->get_component<cVolume>();

	if (hf_terrain)
	{
		map_extent = hf_terrain->extent;

		if (auto height_map_info_fn = hf_terrain->height_map->filename; !height_map_info_fn.empty())
		{
			height_map_info_fn += L".info";
			std::ifstream file(height_map_info_fn);
			if (file.good())
			{
				LineReader info(file);
				info.read_block("sites:");
				unserialize_text(info, &site_positions);
				file.close();
			}
		}

		site_centrality.resize(site_positions.size());
		for (auto i = 0; i < site_positions.size(); i++)
		{
			auto x = abs(site_positions[i].x * 2.f - 1.f);
			auto z = abs(site_positions[i].z * 2.f - 1.f);
			site_centrality[i] = std::make_pair(x * z, i);
		}
		std::sort(site_centrality.begin(), site_centrality.end(), [](const auto& a, const auto& b) {
			return a.first < b.first;
		});
	}
	else if (mc_terrain)
	{
		map_extent = mc_terrain->extent;
	}

	if (type == "random"_h)
	{
		auto plane = Entity::create();
		auto node = plane->add_component<cNode>();
		node->set_scl(vec3(20.f, 1.f, 20.f));
		auto mesh = plane->add_component<cMesh>();
		mesh->set_mesh_and_material(L"plane", L"default");
		e->add_child(plane);

	}
}

vec3 get_map_coord(const vec2& uv)
{
	if (hf_terrain)
		return map_node->pos + map_extent * vec3(uv.x, hf_terrain->height_map->linear_sample(uv).r, uv.y);
	else if (mc_terrain)
		return map_node->pos + map_extent * vec3(uv.x, 1.f - mc_terrain->height_map->linear_sample(uv).r, uv.y);
	return vec3(0.f);
}

vec3 get_map_coord(const vec3& pos)
{
	if (!map_node)
		return pos;
	return get_map_coord(vec2((pos.x - map_node->pos.x) / map_extent.x, (pos.z - map_node->pos.z) / map_extent.z));
}

vec3 get_coord_by_centrality(int i)
{
	if (i < 0)
		i = (int)site_centrality.size() + i;
	return get_map_coord(site_positions[site_centrality[i].second].xz());
}

const uint vision_map_resolution = 2;
const uint vision_map_W = 256 * vision_map_resolution;
const uint vision_map_H = 256 * vision_map_resolution;
static graphics::ImagePtr img_vision = nullptr;
static int vision_map_res_id = -1;
static graphics::BufferPtr vision_stagbuf = nullptr;

static std::map<uint, std::vector<uchar>> visions;

void init_vision()
{
	img_vision = graphics::Image::create(graphics::Format_R8_UNORM, uvec3(vision_map_W, vision_map_H, 1), graphics::ImageUsageSampled | graphics::ImageUsageTransferDst);
	img_vision->change_layout(graphics::ImageLayoutShaderReadOnly);
	vision_map_res_id = sRenderer::instance()->get_texture_res(img_vision->get_view());
	sRenderer::instance()->set_texture_res_name(vision_map_res_id, "VISION");

	vision_stagbuf = graphics::Buffer::create(vision_map_W * vision_map_H, graphics::BufferUsageTransferSrc, graphics::MemoryPropertyHost | graphics::MemoryPropertyCoherent);
	vision_stagbuf->map();
}

void deinit_vision()
{
	if (vision_map_res_id != -1)
		sRenderer::instance()->release_texture_res(vision_map_res_id);
	if (img_vision)
		delete img_vision;
	img_vision = nullptr;
}

bool get_vision(uint faction, const vec3& coord)
{
	auto it = visions.find(faction);
	if (it == visions.end())
		return false;
	auto buf = it->second.data();
	auto c = (ivec2)(coord.xz() * (float)vision_map_resolution);
	if (c.x >= 0 && c.y >= 0 && c.x < vision_map_W && c.y < vision_map_W)
	{
		auto v = buf[c.y * vision_map_W + c.x];
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
	if (hf_terrain)
	{
		for (auto& f : factions)
		{
			if (player1.faction != f.first && multi_player != SinglePlayer && multi_player != MultiPlayerAsHost)
				continue;

			auto it = visions.find(f.first);
			if (it == visions.end())
			{
				it = visions.emplace(f.first, std::vector<uchar>()).first;
				it->second.resize(vision_map_W * vision_map_H);
				if (f.first == FactionCreep)
					memset(it->second.data(), 255, vision_map_W * vision_map_H);
			}

			auto buf = it->second.data();
			if (f.first != FactionCreep) // well, creeps have full vision
			{
				auto changed = false;
				for (auto character : f.second)
				{
					auto pos = character->node->pos;
					auto coord = (ivec2)(pos.xz() * (float)vision_map_resolution);
					if (coord != character->vision_coord)
					{
						changed = true;
						character->vision_coord = coord;
					}
				}

				if (changed)
				{
					memset(buf, 0, vision_map_W * vision_map_H);

					for (auto character : f.second)
					{
						auto pos = character->node->pos;
						auto coord = character->vision_coord;
						auto height = pos.y;
						auto range = character->vision_range;
						auto elev = map_extent.y / 4.f;
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
						sScene::instance()->octree->get_colliding(pos, range / (float)vision_map_resolution, objs);
						std::vector<std::pair<vec2, float>> blockers;
						for (auto n : objs)
						{
							if (n->entity->name == "blocker")
								blockers.emplace_back(n->global_pos().xz(), n->global_scl().x * 0.5f);
						}

						for (auto y = int(coord.y - range); y <= coord.y + range; y++)
						{
							for (auto x = int(coord.x - range); x <= coord.x + range; x++)
							{
								if (x >= 0 && y >= 0 && x < vision_map_W && y < vision_map_H)
								{
									auto mark = false;
									if (get_map_coord(vec2((x + 0.5f) / vision_map_W, (y + 0.5f) / vision_map_H)).y > height) // high elevation
										mark = true;
									else
									{
										for (auto& b : blockers)
										{
											if (distance(vec2(x, y) / (float)vision_map_resolution, b.first) > b.second)
												continue;
											if (distance(vec2(x + 1, y) / (float)vision_map_resolution, b.first) > b.second)
												continue;
											if (distance(vec2(x + 1, y + 1) / (float)vision_map_resolution, b.first) > b.second)
												continue;
											if (distance(vec2(x, y + 1) / (float)vision_map_resolution, b.first) > b.second)
												continue;
											mark = true;
											break;
										}
									}
									if (mark)
										buf[y * vision_map_W + x] = 1;
								}
							}
						}

						auto cast_beam = [&](const std::vector<ivec2>& b) {
							for (auto& p : b)
							{
								if (length((vec2)p) > range)
									break;
								auto c = coord + p;
								if (c.x < 0 || c.y < 0 || c.x >= vision_map_W || c.y >= vision_map_H)
									break;
								auto& dst = buf[c.y * vision_map_W + c.x];
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

					if (f.first == player1.faction)
					{
						graphics::Queue::get()->wait_idle();

						graphics::InstanceCommandBuffer cb;
						memcpy(vision_stagbuf->mapped, visions[player1.faction].data(), vision_stagbuf->size);
						cb->image_barrier(img_vision, {}, graphics::ImageLayoutTransferDst);
						cb->copy_buffer_to_image(vision_stagbuf, img_vision, graphics::BufferImageCopy(uvec3(vision_map_W, vision_map_H, 1)));
						cb->image_barrier(img_vision, {}, graphics::ImageLayoutShaderReadOnly);
						cb.excute();
					}
				}
			}
		}
	}

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		// update objects' visible
		for (auto& pair : objects)
		{
			auto visible_flags = 0;
			for (auto& f : factions)
			{
				auto v = get_vision(f.first, pair.second->entity->get_component<cNode>()->pos);
				if (v)
					visible_flags |= f.first;
			}
			pair.second->set_visible_flags(visible_flags);
		}
	}
}
