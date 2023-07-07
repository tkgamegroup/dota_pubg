#include <flame/graphics/gui.h>
#include <flame/universe/world.h>
#include <flame/universe/octree.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/systems/scene.h>

#include "game.h"
#include "player.h"
#include "map.h"
#include "control.h"
#include "network.h"
#include "presets.h"
#include "ui/ui.h"
#include "entities/ability.h"
#include "entities/buff.h"
#include "entities/item.h"
#include "entities/object.h"
#include "entities/effect.h"
#include "entities/projectile.h"
#include "entities/spawner.h"
#include "entities/character.h"
#include "entities/chest.h"
#include "entities/collider.h"
#include "entities/ai.h"

void MainCamera::init(EntityPtr e)
{
	entity = e;
	if (e)
	{
		node = e->node();
		camera = e->get_component_t<cCamera>();
	}
}

void MainCamera::set_follow_target(const AABB& bounds)
{
	follow_target = fit_camera_to_object(mat3(node->g_qut), camera->fovy, camera->zNear, camera->aspect, bounds);
}

void MainCamera::update()
{
	if (follow_target.y > -1000.f)
	{
		static vec3 velocity = vec3(0.f);
		node->set_pos(smooth_damp(node->pos, follow_target, velocity, 0.6f, 20.f, delta_time));
		if (distance(node->pos, follow_target) < 0.2f)
			follow_target.y = -2000.f;
	}
}

MainCamera main_camera;

EntityPtr root = nullptr;

bool parse_literal(const std::string& str, int& id)
{
	if (SUS::match_head_tail(str, "\"", "\"h"))
	{
		id = sh(str.substr(1, str.size() - 3).c_str());
		return true;
	}
	//else if (SUS::match_head_tail(str, "\"", "\"s"))
	//{
	//	CharacterState state;
	//	TypeInfo::unserialize_t(str.substr(1, str.size() - 3), state);
	//	id = state;
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"b"))
	//{
	//	id = Buff::find(str.substr(1, str.size() - 3));
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"i"))
	//{
	//	id = Item::find(str.substr(1, str.size() - 3));
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"a"))
	//{
	//	id = Ability::find(str.substr(1, str.size() - 3));
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"t"))
	//{
	//	id = Talent::find(str.substr(1, str.size() - 3));
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"ef"))
	//{
	//	id = EffectPreset::find(str.substr(1, str.size() - 4));
	//	return true;
	//}
	//else if (SUS::match_head_tail(str, "\"", "\"pt"))
	//{
	//	id = ProjectilePreset::find(str.substr(1, str.size() - 4));
	//	return true;
	//}
	return false;
}

void enable_game(bool v)
{
	World::instance()->update_components = v;
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

std::filesystem::path get_prefab_path(uint prefab_id)
{
	static std::map<uint, std::filesystem::path> paths;

	static bool first = true;
	if (first)
	{
		for (auto& entry : std::filesystem::recursive_directory_iterator(Path::get("assets")))
		{

		}
	}

	auto it = paths.find(prefab_id);
	if (it == paths.end())
		return L"";
	return it->second;
}

static vec3 get_avaliable_pos(const vec3& pos, float radius, float max_distance, uint times = 20)
{
	auto ret = pos; auto scene = sScene::instance();
	while (times > 0)
	{
		bool ok = true;
		ok = scene->navmesh_nearest_point(ret, vec3(2.f, 4.f, 2.f), ret);
		if (ok)
			ok = scene->navmesh_check_free_space(ret, radius);
		if (ok)
			break;
		ret.xz = pos.xz() + circularRand(max_distance);
		times--;
	}
	if (times == 0)
		ret = vec3(10000.f);
	return ret;
}

void add_player(vec3& pos, uint& faction, uint& prefab_id)
{
	static uint idx = 0;
	/*if (!main_terrain.site_centrality.empty())
		pos = main_terrain.get_coord_by_centrality(-idx - 1);
	else
		*/pos = get_map_coord(vec2(0.5f));
	faction = 1 << (log2i((uint)FactionParty1) + idx);
	//prefab_id = CharacterPreset::find("Dragon Knight");
	idx++;
}

std::vector<cCharacterPtr> find_characters_within_circle(FactionFlags faction, const vec3& pos, float radius)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos.xz(), radius, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
			ret.push_back(chr);
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

std::vector<cCharacterPtr> find_characters_within_sector(FactionFlags faction, const vec3& pos, float inner_radius, float outer_radius, float angle, float rotation)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_colliding(pos.xz(), outer_radius, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
		{
			if (circle_sector_intersect(obj->pos, chr->nav_agent->radius, pos, inner_radius, outer_radius, angle, rotation))
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

std::vector<cCharacterPtr> find_characters_within_camera(FactionFlags faction)
{
	std::vector<cCharacterPtr> ret;

	std::vector<cNodePtr> objs;
	sScene::instance()->octree->get_within_frustum(main_camera.camera->frustum, objs, CharacterTag);
	for (auto obj : objs)
	{
		if (auto chr = obj->entity->get_component_t<cCharacter>(); chr && (chr->faction & faction))
			ret.push_back(chr);
	}

	return ret;
}

cCharacterPtr add_character(const std::filesystem::path& prefab_path, const vec3& _pos, FactionFlags faction, uint id)
{
	auto e = get_prefab(prefab_path);
	float radius = 0.f;
	if (auto nav_agent = e->get_component_t<cNavAgent>(); nav_agent)
		radius = nav_agent->radius;
	auto pos = _pos;
	if (radius > 0.f)
	{
		pos = get_avaliable_pos(pos, radius, 4.f, 20);
		if (pos.x >= 10000.f)
			return nullptr;
	}
	e = e->duplicate();

	auto node = e->node();
	auto object = e->get_component_t<cObject>();
	auto character = e->get_component_t<cCharacter>();
	node->set_pos(pos);
	//object->init(1000 + prefab_id, id);
	characters.push_back(character);
	character->set_faction(faction);
	if (multi_player == MultiPlayerAsHost)
	{
		//auto harvester = e->add_component_t<cNWDataHarvester>();
		//harvester->add_target(th<cCharacter>(), "hp"_h);
		//harvester->add_target(th<cCharacter>(), "hp_max"_h);
		//harvester->add_target(th<cCharacter>(), "mp"_h);
		//harvester->add_target(th<cCharacter>(), "mp_max"_h);
		//harvester->add_target(th<cCharacter>(), "lv"_h);
	}
	root->add_child(e);

	return character;
}

cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, cCharacterPtr target, float speed, uint id)
{
	auto e = get_prefab(prefab_path)->duplicate();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	//object->init(2000 + prefab_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectiles.push_back(projectile);
	projectile->target.set(target);
	projectile->speed = speed;
	root->add_child(e);

	return projectile;
}

cProjectilePtr add_projectile(const std::filesystem::path& prefab_path, const vec3& pos, const vec3& location, float speed, uint id)
{
	auto e = get_prefab(prefab_path)->duplicate();
	e->node()->set_pos(pos);
	auto object = e->get_component_t<cObject>();
	//object->init(2000 + prefab_id, id);
	auto projectile = e->get_component_t<cProjectile>();
	projectiles.push_back(projectile);
	projectile->use_target = false;
	projectile->location = location;
	projectile->speed = speed;
	root->add_child(e);

	return projectile;
}

cEffectPtr add_effect(const std::filesystem::path& prefab_path, const vec3& pos, const quat& qut, float duration, uint id)
{
	auto e = get_prefab(prefab_path)->duplicate();
	auto node = e->node();
	node->set_pos(pos);
	node->set_qut(qut);
	auto object = e->get_component_t<cObject>();
	//object->init(3000 + prefab_id, id);
	auto effect = e->get_component_t<cEffect>();
	effects.push_back(effect);
	effect->duration = duration;
	root->add_child(e);

	return effect;
}

cChestPtr add_chest(const vec3& pos, uint item_id, uint item_num, uint id)
{
	auto e = get_prefab(L"assets\\models\\chest.prefab")->duplicate();
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

bool cGame::get_enable_collider_debugging()
{
	return enable_collider_debugging;
}

void cGame::set_enable_collider_debugging(bool v)
{
	enable_collider_debugging = v;
}

static std::vector<graphics::ImagePtr> preload_images;

cGame::~cGame()
{
	prefabs.clear();
	deinit_ui();

	for (auto img : preload_images)
		graphics::Image::release(img);
}

void cGame::start()
{
	root = entity;
	main_camera.init(root->find_child("main_camera"));
	init_ui();
	init_control();
	init_presets();

	// init players
	player1.faction = FactionParty1;
	player2.faction = FactionParty2;
	player1.init(root->find_child_recursively("player1_town"));
	player2.init(root->find_child_recursively("player2_town"));
	if (player2.town.e)
	{
		if (auto node = player2.town.e->node(); node)
		{
			node->update_transform_from_root();
			player1.town.target_pos = node->global_pos();
		}
	}
	if (player1.town.e)
	{
		if (auto node = player1.town.e->node(); node)
		{
			node->update_transform_from_root();
			player2.town.target_pos = node->global_pos();
		}
	}

	preload_images.push_back(graphics::Image::get(L"assets\\effects\\Fireball.png"));
}

void cGame::update()
{
	removing_dead_effects = true;
	for (auto o : dead_effects)
		o->entity->remove_from_parent();
	dead_effects.clear();
	removing_dead_effects = false;

	removing_dead_projectiles = true;
	for (auto o : dead_projectiles)
		o->entity->remove_from_parent();
	dead_projectiles.clear();
	removing_dead_projectiles = false;

	removing_dead_chests = true;
	for (auto o : dead_chests)
	{
		if (hovering_chest == o)
			hovering_chest = nullptr;
		o->entity->remove_from_parent();
	}
	dead_chests.clear();
	removing_dead_chests = false;

	removing_dead_characters = true;
	for (auto o : dead_characters)
	{
		if (hovering_character == o)
			hovering_character = nullptr;
		o->entity->remove_from_parent();
	}
	dead_characters.clear();
	removing_dead_characters = false;

	player1.update();
	player2.update();

	// naive ai for computer
	for (auto& b : player2.town.buildings)
	{
		if (!b.info->training_actions.empty() && b.trainings.empty())
		{

		}
	}

	if (enable_control)
		update_control();
	if (enable_ui)
		update_ui();
	main_camera.update();
}

cLauncher::~cLauncher()
{
	graphics::gui_callbacks.remove((uint)this);
}

void enter_scene(EntityPtr root)
{
	add_event([root]() {
		graphics::gui_set_clear(false, vec4(0.f));
		root->remove_component_t<cLauncher>();
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

void* cpp_info()
{
	cGame::create((EntityPtr)INVALID_POINTER);
	return nullptr;
}
