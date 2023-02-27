

void MainTerrain::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->node();
		hf_terrain = e->get_component_t<cTerrain>();
		mc_terrain = e->get_component_t<cVolume>();

		if (hf_terrain)
		{
			extent = hf_terrain->extent;

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
			extent = mc_terrain->extent;
		}
	}
}

vec3 MainTerrain::get_coord(const vec2& uv)
{
	if (hf_terrain)
		return node->pos + extent * vec3(uv.x, hf_terrain->height_map->linear_sample(uv).r, uv.y);
	else if (mc_terrain)
		return node->pos + extent * vec3(uv.x, 1.f - mc_terrain->height_map->linear_sample(uv).r, uv.y);
	return vec3(0.f);
}

vec3 MainTerrain::get_coord(const vec3& pos)
{
	return get_coord(vec2((pos.x - node->pos.x) / extent.x, (pos.z - node->pos.z) / extent.z));
}

vec3 MainTerrain::get_coord_by_centrality(int i)
{
	if (i < 0)
		i = (int)site_centrality.size() + i;
	return get_coord(site_positions[site_centrality[i].second].xz());
}
