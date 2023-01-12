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
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

std::vector<Ability> abilities;
std::vector<Talent> talents;

void init_abilities()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\abilities.ini")).sections)
	{
		auto& ability = abilities.emplace_back();
		ability.id = abilities.size() - 1;
		ability.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "icon_name")
				ability.icon_name = e.values[0];
			else if (e.key == "icon_tile_coord")
				ability.icon_tile_coord = s2t<2, uint>(e.values[0]);
			else if (e.key == "description")
				ability.description = e.values[0];
			else if (e.key == "target_type")
				TypeInfo::unserialize_t(e.values[0], ability.target_type);
			else if (e.key == "mp")
			{
				for (auto& t : SUS::split(e.values[0], '/'))
					ability.mp.push_back(s2t<uint>(t));
			}
			else if (e.key == "cd")
			{
				for (auto& t : SUS::split(e.values[0], '/'))
					ability.cd.push_back(s2t<uint>(t));
			}
			else if (e.key == "distance")
			{
				for (auto& t : SUS::split(e.values[0], '/'))
					ability.distance.push_back(s2t<float>(t));
			}
			else if (e.key == "range")
			{
				for (auto& t : SUS::split(e.values[0], '/'))
					ability.range.push_back(s2t<float>(t));
			}
			else if (e.key == "angle")
			{
				for (auto& t : SUS::split(e.values[0], '/'))
					ability.angle.push_back(s2t<float>(t));
			}
			else if (e.key == "parameters")
				read_parameters(ability.parameter_names, ability.parameters, e.values);
			else if (e.key == "active")
				ability.active.build(e.values);
			else if (e.key == "passive")
				ability.passive.build(e.values);
		}
	}

	for (auto& ability : abilities)
	{
		if (!ability.icon_name.empty())
		{
			ability.icon_image = graphics::Image::get(ability.icon_name);
			if (ability.icon_image)
			{
				auto tiles = vec2(ability.icon_image->tiles);
				if (tiles != vec2(0.f))
					ability.icon_uvs = vec4(vec2(ability.icon_tile_coord) / tiles, vec2(ability.icon_tile_coord + 1U) / tiles);
			}
		}
	}

	for (auto& section : parse_ini_file(Path::get(L"assets\\talents.ini")).sections)
	{
		auto& talent = talents.emplace_back();
		talent.id = talents.size() - 1;
		talent.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "layer")
			{
				std::vector<uint> vec;
				for (auto& t : e.values)
				{
					if (auto id = Ability::find(t); id != -1)
						vec.push_back(id);
				}
				talent.ablilities_list.push_back(std::move(vec));
			}
		}
	}
}

int Ability::find(const std::string& name)
{
	for (auto i = 0; i < abilities.size(); i++)
	{
		if (abilities[i].name == name)
			return i;
	}
	return -1;
}

const Ability& Ability::get(uint id)
{
	return abilities[id];
}

int Talent::find(const std::string& name)
{
	for (auto i = 0; i < talents.size(); i++)
	{
		if (talents[i].name == name)
			return i;
	}
	return -1;
}

const Talent& Talent::get(uint id)
{
	return talents[id];
}
