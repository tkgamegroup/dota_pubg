#include <flame/graphics/gui.h>
#include <flame/universe/world.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

#include "game.h"
#include "map.h"
#include "command.h"
#include "network.h"
#include "entities/ability.h"
#include "entities/buff.h"
#include "entities/item.h"
#include "entities/object.h"
#include "entities/effect.h"
#include "entities/projectile.h"
#include "entities/character.h"
#include "entities/chest.h"
#include "entities/collider.h"
#include "entities/ai.h"

bool parse_literal(const std::string& str, int& id)
{
	if (SUS::match_head_tail(str, "\"", "\"h"))
	{
		id = sh(str.substr(1, str.size() - 3).c_str());
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"s"))
	{
		CharacterState state;
		TypeInfo::unserialize_t(str.substr(1, str.size() - 3), state);
		id = state;
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"b"))
	{
		id = Buff::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"i"))
	{
		id = Item::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"a"))
	{
		id = Ability::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"t"))
	{
		id = Talent::find(str.substr(1, str.size() - 3));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"ef"))
	{
		id = EffectPreset::find(str.substr(1, str.size() - 4));
		return true;
	}
	else if (SUS::match_head_tail(str, "\"", "\"pt"))
	{
		id = ProjectilePreset::find(str.substr(1, str.size() - 4));
		return true;
	}
	return false;
}

void enable_game(bool v)
{
	root->world->update_components = v;
	sScene::instance()->enable = v;
}

static std::map<std::filesystem::path, EntityPtr> prefabs;

EntityPtr get_prefab(const std::filesystem::path& _path)
{
	auto path = Path::get(_path);
	auto it = prefabs.find(path);
	if (it == prefabs.end())
	{
		auto e = Entity::create();
		e->load(path);
		it = prefabs.emplace(path, e).first;
	}
	return it->second;
}

void add_player(vec3& pos, uint& faction, uint& preset_id)
{
	static uint idx = 0;
	/*if (!main_terrain.site_centrality.empty())
		pos = main_terrain.get_coord_by_centrality(-idx - 1);
	else
		*/pos = get_map_coord(vec2(0.5f));
	faction = 1 << (log2i((uint)FactionParty1) + idx);
	preset_id = CharacterPreset::find("Dragon Knight");
	idx++;
}

std::vector<cCharacterPtr> find_characters(uint faction, const vec3& pos, float r1, float r0, float central_angle, float direction_angle)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos.xz(), r1, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
		{
			if (central_angle == 360.f ||
				circle_sector_intersect(obj->pos, chr->nav_agent->radius, pos, r0, r1, central_angle, direction_angle))
				ret.push_back(chr);
		}
	}

	// sort them by distances
	std::vector<std::pair<float, cCharacterPtr>> distance_list(ret.size());
	for (auto i = 0; i < ret.size(); i++)
	{
		auto c = ret[i];
		distance_list[i] = std::make_pair(distance(c->node->pos, pos), c);
	}
	std::sort(distance_list.begin(), distance_list.end(), [](const auto& a, const auto& b) {
		return a.first < b.first;
		});
	for (auto i = 0; i < ret.size(); i++)
		ret[i] = distance_list[i].second;
	return ret;
}

cCharacterPtr add_character(uint preset_id, const vec3& pos, uint faction, uint id)
{
	auto& preset = CharacterPreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(1000 + preset_id, id);
	auto character = e->get_component_t<cCharacter>();
	characters.push_back(character);
	character->preset = &preset;
	character->set_faction(faction);
	if (multi_player == MultiPlayerAsHost)
	{
		auto harvester = e->add_component_t<cNWDataHarvester>();
		harvester->add_target(th<cCharacter>(), "hp"_h);
		harvester->add_target(th<cCharacter>(), "hp_max"_h);
		harvester->add_target(th<cCharacter>(), "mp"_h);
		harvester->add_target(th<cCharacter>(), "mp_max"_h);
		harvester->add_target(th<cCharacter>(), "lv"_h);
	}
	root->add_child(e);

	return character;
}

cProjectilePtr add_projectile(uint preset_id, const vec3& pos, cCharacterPtr target, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id)
{
	auto& preset = ProjectilePreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(2000 + preset_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectiles.push_back(projectile);
	projectile->preset = &preset;
	projectile->target.set(target);
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);

	return projectile;
}

cProjectilePtr add_projectile(uint preset_id, const vec3& pos, const vec3& location, float speed, const std::function<void(const vec3&, cCharacterPtr)>& on_end, uint id)
{
	auto& preset = ProjectilePreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	object->init(2000 + preset_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectiles.push_back(projectile);
	projectile->preset = &preset;
	projectile->use_target = false;
	projectile->location = location;
	projectile->speed = speed;
	projectile->on_end = on_end;
	root->add_child(e);

	return projectile;
}

cEffectPtr add_effect(uint preset_id, const vec3& pos, const vec3& eul, float duration, uint id)
{
	auto& preset = EffectPreset::get(preset_id);
	auto e = get_prefab(preset.path)->copy();
	auto node = e->node();
	node->set_pos(pos);
	node->set_eul(eul);
	auto object = e->get_component_t<cObject>();
	object->init(3000 + preset_id, id);
	auto effect = e->get_component_t<cEffect>();
	effects.push_back(effect);
	effect->preset = &preset;
	effect->duration = duration;
	root->add_child(e);

	return effect;
}

cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num, uint id)
{
	auto e = get_prefab(L"assets\\models\\chest.prefab")->copy();
	e->node()->set_pos(get_map_coord(pos));
	root->add_child(e);
	auto object = e->get_component_t<cObject>();
	object->init(4000, id);
	auto chest = e->get_component_t<cChest>();
	chests.push_back(chest);
	chest->item_id = item_id;
	chest->item_num = item_num;

	return chest;
}

void teleport(cCharacterPtr character, const vec3& location)
{
	character->node->set_pos(location);
	character->nav_agent->update_pos();
}

struct cGameCreate : cGame::Create
{
	cGamePtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cGame;
	}
}cGame_create;
cGame::Create& cGame::create = cGame_create;

cLauncher::~cLauncher()
{
	graphics::gui_callbacks.remove((uint)this);
}

void enter_scene(EntityPtr root)
{
	add_event([root]() {
		graphics::gui_set_clear(false, vec4(0.f));
		root->remove_component<cLauncher>();
		root->load(L"assets\\main.prefab");
		return false;
		});
}

void cLauncher::start()
{
	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		if (ImGui::Button("Single Player"))
		{
			enter_scene(entity);
		}
		if (ImGui::Button("Create Local Server"))
		{
			start_server();
			enter_scene(entity);
		}
		if (ImGui::Button("Join Local Server"))
		{
			join_server();
			if (so_client)
			{
				multi_player = MultiPlayerAsClient;
				enter_scene(entity);
			}
		}
		}, (uint)this);
}

struct cLauncherCreate : cLauncher::Create
{
	cLauncherPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cLauncher;
	}
}cLauncher_create;
cLauncher::Create& cLauncher::create = cLauncher_create;

extern "C" EXPORT void* cpp_info()
{
	auto uinfo = universe_info(); // references universe module explicitly
	cLauncher::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cGame::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cObject::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCharacter::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cProjectile::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cEffect::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cSectorCollider::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cChest::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cCreepAI::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	cNWDataHarvester::create((EntityPtr)INVALID_POINTER); // references create function explicitly
	return nullptr;
}

extern "C" EXPORT void set_editor_info(Entity * *p_selecting_entity, bool* p_control)
{
	in_editor = true;
	//editor_p_selecting_entity = p_selecting_entity;
	//editor_p_control = p_control;
}
