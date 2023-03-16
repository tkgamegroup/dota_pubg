#include "ability.h"
#include "character.h"
#include "buff.h"
#include "projectile.h"
#include "effect.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/components/nav_agent.h>

std::vector<Talent> talents;

void init_abilities()
{
	//for (auto& section : parse_ini_file(Path::get(L"assets\\abilities.ini")).sections)
	//{
	//	auto& ability = abilities.emplace_back();
	//	ability.id = abilities.size() - 1;
	//	ability.name = section.name;
	//	for (auto& e : section.entries)
	//	{
	//		switch (e.key_hash)
	//		{
	//		case "icon_name"_h:
	//			ability.icon_name = e.values[0];
	//			break;
	//		case "icon_tile_coord"_h:
	//			ability.icon_tile_coord = s2t<2, uint>(e.values[0]);
	//			break;
	//		case "description"_h:
	//			ability.description = e.values[0];
	//			break;
	//		case "target_type"_h:
	//			TypeInfo::unserialize_t(e.values[0], ability.target_type);
	//			break;
	//		case "mp"_h:
	//			for (auto& t : SUS::split(e.values[0], '/'))
	//				ability.mp.push_back(s2t<uint>(t));
	//			break;
	//		case "cd"_h:
	//			for (auto& t : SUS::split(e.values[0], '/'))
	//				ability.cd.push_back(s2t<uint>(t));
	//			break;
	//		case "distance"_h:
	//			for (auto& t : SUS::split(e.values[0], '/'))
	//				ability.distance.push_back(s2t<float>(t));
	//			break;
	//		case "range"_h:
	//			for (auto& t : SUS::split(e.values[0], '/'))
	//				ability.range.push_back(s2t<float>(t));
	//			break;
	//		case "angle"_h:
	//			ability.angle = s2t<float>(e.values[0]);
	//			break;
	//		case "start_radius"_h:
	//			ability.start_radius = s2t<float>(e.values[0]);
	//			break;
	//		case "parameters"_h:
	//			read_parameters(ability.parameter_names, ability.parameters, e.values);
	//			break;
	//		case "active"_h:
	//			ability.active.build(e.values);
	//			break;
	//		case "passive"_h:
	//			ability.passive.build(e.values);
	//			break;
	//		}
	//	}
	//}

	//for (auto& ability : abilities)
	//{
	//	if (!ability.icon_name.empty())
	//	{
	//		ability.icon_image = graphics::Image::get(ability.icon_name);
	//		if (ability.icon_image)
	//		{
	//			auto tiles = vec2(ability.icon_image->tiles);
	//			if (tiles != vec2(0.f))
	//				ability.icon_uvs = vec4(vec2(ability.icon_tile_coord) / tiles, vec2(ability.icon_tile_coord + 1U) / tiles);
	//		}
	//	}
	//}

	//for (auto& section : parse_ini_file(Path::get(L"assets\\talents.ini")).sections)
	//{
	//	auto& talent = talents.emplace_back();
	//	talent.id = talents.size() - 1;
	//	talent.name = section.name;
	//	for (auto& e : section.entries)
	//	{
	//		switch (e.key_hash)
	//		{
	//		case "layer"_h:
	//		{
	//			std::vector<uint> vec;
	//			for (auto& t : e.values)
	//			{
	//				if (auto id = Ability::find(t); id != -1)
	//					vec.push_back(id);
	//			}
	//			talent.ablilities_list.push_back(std::move(vec));
	//		}
	//			break;
	//		}
	//	}
	//}
}
