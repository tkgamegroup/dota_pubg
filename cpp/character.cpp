#include "character.h"
#include "object.h"
#include "item.h"
#include "ability.h"
#include "buff.h"
#include "projectile.h"
#include "chest.h"
#include "network.h"
#include "views/view_ability.h"

#include <flame/graphics/image.h>
#include <flame/graphics/gui.h>
#include <flame/universe/octree.h>
#include <flame/universe/entity.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/camera.h>
#include <flame/universe/components/armature.h>
#include <flame/universe/components/nav_agent.h>
#include <flame/universe/components/audio_source.h>
#include <flame/universe/systems/scene.h>
#include <flame/universe/systems/input.h>
#include <flame/universe/systems/renderer.h>

CharacterCommand::CharacterCommand(uint type, cCharacterPtr character) :
	type(type),
	character(character)
{
	character->command.reset(this);
	character->action = ActionNone;
}

CharacterCommandIdle::CharacterCommandIdle(cCharacterPtr character) :
	CharacterCommand("Idle"_h, character)
{
}

void CharacterCommandIdle::update()
{
	character->action = ActionNone;
}

CharacterCommandMoveTo::CharacterCommandMoveTo(cCharacterPtr character, const vec3& _location) :
	CharacterCommand("MoveTo"_h, character)
{
	location = _location;
}

void CharacterCommandMoveTo::update()
{
	if (character->process_approach(location))
	{
		character->nav_agent->stop();
		new CharacterCommandIdle(character);
	}
}

CharacterCommandAttackTarget::CharacterCommandAttackTarget(cCharacterPtr character, cCharacterPtr _target) :
	CharacterCommand("AttackTarget"_h, character)
{
	target.set(_target);
}

void CharacterCommandAttackTarget::update()
{
	if (!target.obj)
		new CharacterCommandIdle(character);
	else
		character->process_attack_target(target.obj);
}

CharacterCommandAttackLocation::CharacterCommandAttackLocation(cCharacterPtr character, const vec3& _location) :
	CharacterCommand("AttackLocation"_h, character)
{
	location = _location;
}

void CharacterCommandAttackLocation::update()
{
	if (!target.obj && character->action != ActionAttack)
	{
		if (character->search_timer <= 0.f)
		{
			auto enemies = find_characters(character->node->pos, max(character->preset->atk_distance, 5.f), ~character->faction);
			if (!enemies.empty())
				target.set(enemies.front());

			character->search_timer = enemies.empty() ? 0.1f : 1.f + linearRand(0.f, 0.05f);
		}
	}

	if (target.obj)
		character->process_attack_target(target.obj);
	else
	{
		if (character->process_approach(location))
			character->action = ActionNone;
	}
}

CharacterCommandPickUp::CharacterCommandPickUp(cCharacterPtr character, cChestPtr _target) :
	CharacterCommand("PickUp"_h, character)
{
	target.set(_target);
}

void CharacterCommandPickUp::update()
{
	if (!target.obj)
		new CharacterCommandIdle(character);
	else
	{
		if (character->process_approach(target.obj->node->pos, 1.5f))
		{
			if (character->gain_item(target.obj->item_id, target.obj->item_num))
			{
				auto entity = target.obj->entity;
				add_event([entity]() {
					entity->remove_from_parent();
					return false;
				});
			}

			character->nav_agent->stop();
			new CharacterCommandIdle(character);
		}
	}
}

CharacterCommandCastAbility::CharacterCommandCastAbility(cCharacterPtr character, AbilityInstance* ins) :
	CharacterCommand("CastAbility"_h, character),
	ins(ins)
{
}

void CharacterCommandCastAbility::update()
{
	character->process_cast_ability(ins, vec3(0.f), nullptr);
}

CharacterCommandCastAbilityToLocation::CharacterCommandCastAbilityToLocation(cCharacterPtr character, AbilityInstance* ins, const vec3& _location) :
	CharacterCommand("CastAbilityToLocation"_h, character),
	ins(ins)
{
	location = _location;
}

void CharacterCommandCastAbilityToLocation::update()
{
	character->process_cast_ability(ins, location, nullptr);
}

CharacterCommandCastAbilityToTarget::CharacterCommandCastAbilityToTarget(cCharacterPtr character, AbilityInstance* ins, cCharacterPtr _target) :
	CharacterCommand("CastAbilityToTarget"_h, character),
	ins(ins)
{
	target.set(_target);
}

void CharacterCommandCastAbilityToTarget::update()
{
	if (!target.obj)
		new CharacterCommandIdle(character);
	else
		character->process_cast_ability(ins, vec3(0.f), target.obj);
}

std::vector<CharacterPreset> character_presets;

void init_characters()
{
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Dummy";
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Dragon Knight";
		preset.path = L"assets\\characters\\dragon_knight\\main.prefab";
		preset.exp_base = 120;
		preset.hp = 500;
		preset.mp = 100;
		preset.atk = 56;
		preset.atk_interval = 1.7f;
		preset.atk_time = 0.8f;
		preset.atk_point = 0.5f;
		preset.cast_time = 0.5f;
		preset.cast_point = 0.3f;
		preset.talents.push_back("Warrior");
		preset.talents.push_back("Laya Knight");
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Life Stealer";
		preset.path = L"assets\\characters\\life_stealer\\main.prefab";
		preset.exp_base = 200;
		preset.hp = 200;
		preset.mp = 50;
		preset.atk = 35;
		preset.atk_interval = 1.7f;
		preset.atk_time = 0.875f;
		preset.atk_point = 0.39f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Slark";
		preset.path = L"assets\\characters\\slark\\main.prefab";
		preset.exp_base = 200;
		preset.hp = 200;
		preset.mp = 50;
		preset.atk = 37;
		preset.atk_interval = 1.7f;
		preset.atk_time = 0.8f;
		preset.atk_point = 0.5f;
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Spiderling";
		preset.path = L"assets\\characters\\spiderling\\main.prefab";
		preset.exp_base = 200;
		preset.hp = 50;
		preset.mp = 0;
		preset.atk = 6;
		preset.atk_interval = 1.35f;
		preset.atk_time = 0.633f;
		preset.atk_point = 0.5f;
		preset.mov_sp = 400;
		preset.abilities.emplace_back("Stinger", 1);
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Treant";
		preset.path = L"assets\\characters\\treant\\main.prefab";
		preset.exp_base = 800;
		preset.hp = 200;
		preset.mp = 200;
		preset.atk = 15;
		preset.atk_interval = 1.6f;
		preset.atk_time = 0.958f;
		preset.atk_point = 0.467f;
		preset.cast_time = 2.f;
		preset.cast_point = 1.95f;
		preset.mov_sp = 100;
		preset.abilities.emplace_back("Recover", 1);
		preset.drop_items.emplace_back(Item::find("Berry"), 30, 1, 3);
		preset.drop_items.emplace_back(Item::find("Mint"), 5, 1, 1);
	}
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = "Boar";
		preset.path = L"assets\\characters\\boar\\main.prefab";
		preset.exp_base = 1000;
		preset.hp = 300;
		preset.mp = 100;
		preset.atk = 20;
		preset.atk_interval = 1.25f;
		preset.atk_time = 0.958f;
		preset.atk_point = 0.5f;
		preset.mov_sp = 100;
		preset.abilities.emplace_back("Roar", 1);
	}

	for (auto& section : parse_ini_file(Path::get(L"assets\\characters.ini")).sections)
	{
		auto& preset = character_presets.emplace_back();
		preset.id = character_presets.size() - 1;
		preset.name = section.name;
		for (auto& e : section.entries)
		{
			if (e.key == "path")
				preset.path = e.value;
			else if (e.key == "exp_base")
				preset.exp_base = s2t<uint>(e.value);
			else if (e.key == "hp")
				preset.hp = s2t<uint>(e.value);
			else if (e.key == "mp")
				preset.mp = s2t<uint>(e.value);
			else if (e.key == "atk")
				preset.atk = s2t<uint>(e.value);
			else if (e.key == "atk_distance")
				preset.atk_distance = s2t<float>(e.value);
			else if (e.key == "atk_interval")
				preset.atk_interval = s2t<float>(e.value);
			else if (e.key == "atk_time")
				preset.atk_time = s2t<float>(e.value);
			else if (e.key == "atk_point")
				preset.atk_point = s2t<float>(e.value);
			else if (e.key == "atk_projectile_preset")
				preset.atk_projectile_preset = s2t<int>(e.value);
			else if (e.key == "cast_time")
				preset.cast_time = s2t<float>(e.value);
			else if (e.key == "cast_point")
				preset.cast_point = s2t<float>(e.value);
			else if (e.key == "phy_def")
				preset.phy_def = s2t<uint>(e.value);
			else if (e.key == "mag_def")
				preset.mag_def = s2t<uint>(e.value);
			else if (e.key == "hp_reg")
				preset.hp_reg = s2t<uint>(e.value);
			else if (e.key == "mp_reg")
				preset.mp_reg = s2t<uint>(e.value);
			else if (e.key == "mov_sp")
				preset.mov_sp = s2t<uint>(e.value);
			else if (e.key == "atk_sp")
				preset.atk_sp = s2t<uint>(e.value);
			else if (e.key == "abilities")
				;
		}
	}
}

int CharacterPreset::find(const std::string& name)
{
	for (auto i = 0; i < character_presets.size(); i++)
	{
		if (character_presets[i].name == name)
			return i;
	}
	return -1;
}

const CharacterPreset& CharacterPreset::get(uint id)
{
	return character_presets[id];
}

std::vector<cCharacterPtr> characters;
std::map<uint, std::vector<cCharacterPtr>> factions;

void cCharacter::set_faction(uint _faction)
{
	if (faction == _faction)
		return;
	if (faction == 0) // first time
		characters.push_back(this);
	std::erase_if(factions[faction], [this](const auto& i) {
		return i == this;
	});
	faction = _faction;
	factions[faction].push_back(this);

	if (nav_agent)
		nav_agent->separation_group = faction;
}

void cCharacter::set_lv(uint v)
{
	if (lv == v)
		return;
	lv = v;
	data_changed("lv"_h);
}

void cCharacter::set_exp(uint v)
{
	if (exp == v)
		return;
	exp = v;
	data_changed("exp"_h);
}

void cCharacter::set_exp_max(uint v)
{
	if (exp_max == v)
		return;
	exp_max = v;
	data_changed("exp_max"_h);
}

void cCharacter::set_hp(uint v)
{
	if (hp == v)
		return;
	hp = v;
	data_changed("hp"_h);
}

void cCharacter::set_hp_max(uint v)
{
	if (hp_max == v)
		return;
	hp_max = v;
	data_changed("hp_max"_h);
}

void cCharacter::set_mp(uint v)
{
	if (mp == v)
		return;
	mp = v;
	data_changed("mp"_h);
}

void cCharacter::set_mp_max(uint v)
{
	if (mp_max == v)
		return;
	mp_max = v;
	data_changed("mp_max"_h);
}

void cCharacter::set_atk_type(uchar v)
{
	if (atk_type == v)
		return;
	atk_type = v;
	data_changed("atk_type"_h);
}

void cCharacter::set_atk(uint v)
{
	if (atk == v)
		return;
	atk = v;
	data_changed("atk"_h);
}

void cCharacter::set_phy_def(uint v)
{
	if (phy_def == v)
		return;
	phy_def = v;
	data_changed("phy_def"_h);
}

void cCharacter::set_mag_def(uint v)
{
	if (mag_def == v)
		return;
	mag_def = v;
	data_changed("mag_def"_h);
}

void cCharacter::set_mov_sp(uint v)
{
	if (mov_sp == v)
		return;
	mov_sp = v;
	data_changed("mov_sp"_h);
}

void cCharacter::set_atk_sp(uint v)
{
	if (atk_sp == v)
		return;
	atk_sp = v;
	data_changed("atk_sp"_h);
}

cCharacter::~cCharacter()
{
	node->measurers.remove("character"_h);

	if (armature)
		armature->playing_callbacks.remove("character"_h);

	if (main_player.character == this)
	{
		main_player.entity = nullptr;
		main_player.node = nullptr;
		main_player.nav_agent = nullptr;
		main_player.character = nullptr;
	}

	std::erase_if(characters, [this](const auto& i) {
		return i == this;
	});
	std::erase_if(factions[faction], [this](const auto& i) {
		return i == this;
	});
}

void cCharacter::on_init()
{
	nav_agent->separation_group = faction;

	node->measurers.add([this](AABB* ret) {
		if (!nav_agent)
			return false;
		auto radius = nav_agent->radius;
		auto height = nav_agent->height;
		*ret = AABB(AABB(vec3(-radius, 0.f, -radius), vec3(radius, height, radius)).get_points(node->transform));
		return true;
	}, "character"_h);
}

void cCharacter::inflict_damage(cCharacterPtr target, DamageType type, uint value)
{
	value *= linearRand(0.8f, 1.2f);
	uint def;
	switch (type)
	{
	case PhysicalDamage:
		def = target->phy_def;
		break;
	case MagicDamage:
		def = target->mag_def;
		break;
	}
	;
	if (target->take_damage(type, value))
		gain_exp(target->exp_max * 0.15f);

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (target == main_player.character || this == main_player.character)
			add_floating_tip(target->node->pos + vec3(0.f, 0.8f, 0.f), str(value), cvec4(255));
	}
}

bool cCharacter::take_damage(DamageType type, uint value)
{
	if (dead_flag > 0)
		return false;
	if (hp > value)
	{
		set_hp(hp - value);
		return false;
	}

	die();
	return true;
}

void cCharacter::restore_hp(uint value)
{
	set_hp(min(hp + value, hp_max));
}

void cCharacter::restore_mp(uint value)
{
	set_hp(min(mp + value, mp_max));
}

void cCharacter::gain_exp(uint v)
{
	if (exp_max == 0)
		return;
	exp += v;

	auto old_lv = lv;
	while (exp_max > 0 && exp >= exp_max)
	{
		exp -= exp_max;
		lv++;
		ability_points++;
		exp_max *= 1.1f;
		stats_dirty = true;

	}

	if (old_lv != lv)
	{
		view_ability.modal = true;
		view_ability.open();
		audio_source->play("level_up"_h);

		enable_game(false);
	}
}

bool cCharacter::gain_item(uint id, uint num)
{
	for (auto& ins : inventory)
	{
		if (!ins)
		{
			ins.reset(new ItemInstance);
			ins->id = id;
			ins->num = num;
			stats_dirty = true;
			return true;
		}
	}
	return false;
}

bool cCharacter::gain_ability(uint id, uint lv)
{
	for (auto& ins : abilities)
	{
		if (ins->id == id)
			return false;
	}

	auto ins = new AbilityInstance;
	ins->id = id;
	ins->lv = lv;
	abilities.emplace_back(ins);
	stats_dirty = true;
	return true;
}

bool cCharacter::gain_talent(uint id)
{
	for (auto tid : talents)
	{
		if (tid == id)
			return false;
	}

	talents.push_back(id);
	auto& talent = Talent::get(id);
	for (auto& layer : talent.ablilities_list)
	{
		for (auto id : layer)
			gain_ability(id, 0);
	}
	return true;
}

void cCharacter::use_item(ItemInstance* ins)
{
	auto& item = Item::get(ins->id);
	if (item.active.empty())
		return;
	if (item.type == ItemConsumable)
	{
		if (ins->num == 0)
			return;
		ins->num--;
		if (ins->num == 0)
		{
			for (auto i = 0; i < inventory.size(); i++)
			{
				if (inventory[i].get() == ins)
				{
					inventory[i].reset(nullptr);
					break;
				}
			}
		}
	}
	for (auto& c : item.active)
		c.execute(this, nullptr, vec3(0.f), item.parameters, 0);
}

void cCharacter::cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target)
{
	if (ins->cd_timer > 0.f)
		return;
	auto& ability = Ability::get(ins->id);
	if (mp < ability.mp)
		return;

	for (auto& c : ability.active)
		c.execute(this, target, location, ability.parameters, ins->lv);

	ins->cd_max = ability.cd;
	ins->cd_timer = ins->cd_max;
	set_mp(mp - ability.mp);
}

void cCharacter::add_buff(uint id, float time, uint lv, bool replace)
{
	BuffInstance* ins = nullptr;
	if (replace)
	{
		for (auto& i : buffs)
		{
			if (i->id == id)
			{
				ins = i.get();
				break;
			}
		}
	}
	if (!ins)
	{
		ins = new BuffInstance;
		buffs.emplace_back(ins);
	}
	auto& buff = Buff::get(id);
	ins->id = id;
	ins->lv = lv;
	ins->timer = time;
	ins->t0 = time;
	ins->interval = buff.interval;
	ins->duration = time;
	stats_dirty = true;
}

bool cCharacter::add_marker(uint hash, float time)
{
	auto it = markers.find(hash);
	if (it == markers.end())
	{
		markers.emplace(hash, std::make_pair(time, 0));
		return true;
	}
	return false;
}

void cCharacter::die()
{
	if (dead_flag > 0)
		return;
	set_hp(0);
	dead_flag = 1;
}

void cCharacter::start()
{
	entity->tag |= CharacterTag;

	auto e = entity;
	while (e)
	{
		if (armature = e->get_component_t<cArmature>(); armature)
			break;
		if (e->children.empty())
			break;
		e = e->children[0].get();
	}

	if (!preset)
		preset = &character_presets[CharacterPreset::find("Dummy")];
	auto ppath = preset->path.parent_path();

	std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
	audio_buffer_names.emplace_back(L"assets\\level_up.wav", "level_up");
	if (auto path = ppath / L"move.wav"; std::filesystem::exists(path))
		audio_buffer_names.emplace_back(path, "move");
	if (auto path = ppath / L"attack_precast.wav"; std::filesystem::exists(path))
		audio_buffer_names.emplace_back(path, "attack_precast");
	if (auto path = ppath / L"attack_hit.wav"; std::filesystem::exists(path))
		audio_buffer_names.emplace_back(path, "attack_hit");
	audio_source->set_buffer_names(audio_buffer_names);

	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		for (auto& i : preset->abilities)
		{
			if (auto id = Ability::find(i.name); id != -1)
				gain_ability(id, i.lv);
		}
		for (auto& i : preset->talents)
		{
			if (auto id = Talent::find(i); id != -1)
				gain_talent(Talent::find(i));
		}
	}

	inventory.resize(16);

	if (!command)
		new CharacterCommandIdle(this);
}

bool cCharacter::process_approach(const vec3& target, float dist, float ang)
{
	if (state & StateStun)
	{
		nav_agent->stop();
		action = ActionNone;
		return false;
	}

	move_speed = max(0.1f, mov_sp / 100.f);
	nav_agent->set_target(target);
	nav_agent->set_speed_scale(move_speed);

	if ((nav_agent->reached_pos == target || nav_agent->dist < dist) && (ang <= 0.f || abs(nav_agent->ang_diff) < ang))
		return true;
	audio_source->play("move"_h);
	action = ActionMove;
	return false;
}

void cCharacter::process_attack_target(cCharacterPtr target)
{
	if (state & StateStun)
	{
		nav_agent->stop();
		action = ActionNone;
		return;
	}

	if (action == ActionAttack)
	{
		nav_agent->set_speed_scale(0.f);
		nav_agent->set_target(target->node->pos);

		if (attack_hit_timer > 0.f)
		{
			attack_hit_timer -= delta_time;
			if (attack_hit_timer <= 0.f)
			{
				if (distance(node->pos, target->node->pos) <= preset->atk_distance + 3.5f)
				{
					auto attack = [this](cCharacterPtr target) {
						auto damage = atk;
						inflict_damage(target, (DamageType)atk_type, damage);
						if (target->dead_flag == 0)
						{
							static ParameterPack parameters;
							parameters["damage_type"_h].push_back({ .i = atk_type });
							parameters["damage"_h].push_back({ .u = damage });
							for (auto& ef : attack_effects)
							{
								for (auto& c : ef)
									c.execute(this, target, vec3(0.f), parameters, 0);
							}
						}

						audio_source->play("attack_hit"_h);
					};
					if (preset->atk_projectile_preset != -1)
					{
						add_projectile(preset->atk_projectile_preset,
							node->pos + vec3(0.f, nav_agent->height * 0.5f, 0.f), target, 6.f,
							[&](const vec3&, cCharacterPtr t) {
								if (t)
									attack(t);
							});
					}
					else
						attack(target);
				}

				attack_interval_timer = (preset->atk_interval - preset->atk_point) / attack_speed;
			}
		}

		if (attack_timer > 0.f)
		{
			attack_timer -= delta_time;
			if (attack_timer <= 0.f)
				action = ActionNone;
		}
	}
	else
	{
		auto approached = process_approach(target->node->pos, preset->atk_distance, 60.f);
		if (attack_interval_timer <= 0.f)
		{
			if (approached)
			{
				attack_speed = max(0.01f, atk_sp / 100.f); 
				attack_hit_timer = preset->atk_point / attack_speed;
				attack_timer = preset->atk_time / attack_speed;
				action = ActionAttack;

				audio_source->play("attack_precast"_h);
			}
		}
		else
		{
			if (approached)
			{
				action = ActionNone;
				nav_agent->set_speed_scale(0.f);
			}
		}
	}
}

void cCharacter::process_cast_ability(AbilityInstance* ins, const vec3& location, cCharacterPtr target)
{
	if (state & StateStun)
	{
		cast_timer = -1.f;
		nav_agent->stop();
		action = ActionNone;
		return;
	}

	auto& ability = Ability::get(ins->id);
	auto pos = target ? target->node->pos : location;

	auto approached = ability.target_type == TargetNull ? true : process_approach(pos, ability.distance, 15.f);
	if (cast_timer > 0.f)
	{
		cast_timer -= delta_time;
		if (cast_timer <= 0.f)
		{
			cast_ability(ins, pos, target);
			nav_agent->stop();
			new CharacterCommandIdle(this);
		}
		else
		{
			action = ActionCast;
			nav_agent->set_speed_scale(0.f);
		}
	}
	else
	{
		if (approached)
		{
			if (ability.cast_time == 0.f)
			{
				cast_ability(ins, pos, target);
				nav_agent->stop();
				new CharacterCommandIdle(this);
			}
			else
			{
				cast_speed = preset->cast_time / ability.cast_time;
				cast_timer = preset->cast_point / cast_speed;
			}
		}
	}
}

void cCharacter::update()
{
	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
	{
		if (dead_flag > 0)
		{
			if (dead_flag == 1)
			{
				if (!preset->drop_items.empty())
				{
					std::vector<std::pair<uint, uint>> drops;
					for (auto& d : preset->drop_items)
					{
						if (linearRand(0U, 100U) < d.probability)
							drops.emplace_back(d.id, linearRand(d.num_min, d.num_max));
					}
					auto p = node->pos;
					for (auto& d : drops)
						add_chest(main_terrain.get_coord(p + vec3(linearRand(-0.3f, +0.3f), 0.f, linearRand(-0.3f, +0.3f))), d.first, d.second);
				}

				add_event([this]() {
					entity->remove_from_parent();
					return false;
				});

				dead_flag = 2;
			}

			return;
		}

		if (stats_dirty)
		{
			if (exp_max == 0)
				exp_max = preset->exp_base;

			state = StateNormal;

			auto old_hp_max = hp_max;
			auto old_mp_max = mp_max;
			hp_max = preset->hp;
			mp_max = preset->hp;

			atk_type = PhysicalDamage;
			atk = preset->atk;

			phy_def = preset->phy_def;
			mag_def = preset->mag_def;
			hp_reg = preset->hp_reg;
			mp_reg = preset->mp_reg;
			mov_sp = preset->mov_sp;
			atk_sp = preset->atk_sp;

			attack_effects.clear();
			for (auto& ins : abilities)
			{
				if (ins && ins->lv > 0)
				{
					auto& ability = Ability::get(ins->id);
					for (auto& c : ability.passive)
						c.execute(this, nullptr, vec3(0.f), ability.parameters, ins->lv);
				}
			}
			for (auto& ins : buffs)
			{
				auto& buff = Buff::get(ins->id);
				for (auto& c : buff.passive)
					c.execute(this, nullptr, vec3(0.f), buff.parameters, ins->lv);
			}

			if (hp_max != old_hp_max)
			{
				set_hp(hp * (float)hp_max / old_hp_max);
				data_changed("hp_max"_h);
			}
			if (mp_max != old_hp_max)
			{
				set_mp(mp * (float)mp_max / old_mp_max);
				data_changed("mp_max"_h);
			}

			stats_dirty = false;
		}

		if (regeneration_timer > 0)
			regeneration_timer -= delta_time;
		else
		{
			set_hp(min(hp + hp_reg, hp_max));
			set_mp(min(mp + mp_reg, mp_max));
			regeneration_timer = 1.f;
		}

		if (search_timer > 0)
			search_timer -= delta_time;
		if (attack_interval_timer > 0)
			attack_interval_timer -= delta_time;

		for (auto& ins : abilities)
		{
			if (ins && ins->cd_timer > 0.f)
				ins->cd_timer -= delta_time;
		}
		for (auto it = buffs.begin(); it != buffs.end();)
		{
			auto& ins = *it;
			auto& buff = Buff::get(ins->id);

			if (ins->t0 - ins->timer > ins->duration)
			{
				for (auto& c : buff.continuous)
					c.execute(this, nullptr, vec3(0.f), buff.parameters, ins->lv);
				ins->t0 = ins->timer;
			}

			if (ins->timer > 0)
			{
				ins->timer -= delta_time;
				if (ins->timer <= 0)
				{
					it = buffs.erase(it);
					stats_dirty = true;
					continue;
				}
			}
			it++;
		}
		for (auto it = markers.begin(); it != markers.end();)
		{
			if (it->second.first > 0.f)
			{
				it->second.first -= delta_time;
				if (it->second.first <= 0.f)
				{
					it = markers.erase(it);
					continue;
				}
			}
			it++;
		}

		if (command)
			command->update();
	}

	if (armature)
	{
		switch (action)
		{
		case ActionNone:
			armature->loop = true;
			armature->playing_speed = 1.f;
			armature->play("idle"_h);
			break;
		case ActionMove:
			armature->loop = true;
			armature->playing_speed = move_speed;
			armature->play("run"_h);
			break;
		case ActionAttack:
			armature->loop = false;
			armature->playing_speed = attack_speed;
			armature->play("attack"_h);
			break;
		case ActionCast:
			armature->loop = false;
			armature->playing_speed = cast_speed;
			armature->play("cast"_h);
			break;
		}
	}
}

struct cCharacterCreate : cCharacter::Create
{
	cCharacterPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cCharacter;
	}
}cCharacter_create;
cCharacter::Create& cCharacter::create = cCharacter_create;
