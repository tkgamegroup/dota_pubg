#pragma once

#include "main.h"

#include <flame/foundation/network.h>

enum MultiPlayerType
{
	SinglePlayer,
	MultiPlayerAsHost,
	MultiPlayerAsClient
};

enum nwMessage
{
	nwNewPlayerInfo,
	nwAddCharacter,
	nwRemoveCharacter,
	nwUpdateCharacter,
	nwCommandCharacter,
	nwAddProjectile,
	nwRemoveProjectile,
	nwAddChest,
	nwRemoveChest
};

struct nwNewPlayerInfoStruct
{
	uint faction;
	uint character_id;
};

struct nwAddCharacterStruct
{
	uint preset_id;
	uint id;
	uint faction;
	vec3 pos;
};

struct nwRemoveCharacterStruct
{
	uint id;
};

struct nwUpdateCharacterStruct
{
	uint id;
	vec3 pos;
	float yaw;
	uint action;
};

struct nwCommandCharacterStruct
{
	uint id;
	uint type;
	vec3 location;
	uint target;
};

struct nwAddProjectileStruct
{
	uint preset_id;
	uint id;
	vec3 location;
	uint target;
	vec3 pos;
	float speed;
};

struct nwRemoveProjectileStruct
{
	uint id;
};

struct nwAddChestStruct
{
	uint id;
	vec3 pos;
	uint item_id;
	uint item_num;
};

struct nwRemoveChestStruct
{
	uint id;
};

template <typename T>
struct PeedingActions
{
	std::mutex mtx;
	std::vector<T> actions;
};

extern PeedingActions<void*>					peeding_add_players;
extern PeedingActions<nwAddCharacterStruct>		peeding_add_characters;
extern PeedingActions<nwRemoveCharacterStruct>	peeding_remove_characters;
extern PeedingActions<nwUpdateCharacterStruct>	peeding_update_characters;
extern PeedingActions<nwCommandCharacterStruct>	peeding_command_characters;
extern PeedingActions<nwAddProjectileStruct>	peeding_add_projectiles;
extern PeedingActions<nwRemoveProjectileStruct>	peeding_remove_projectiles;
extern PeedingActions<nwAddChestStruct>			peeding_add_chests;
extern PeedingActions<nwRemoveChestStruct>		peeding_remove_chests;

extern MultiPlayerType multi_player;
extern network::ClientPtr nw_client;
extern network::ServerPtr nw_server;
extern std::map<uint, std::vector<void*>> nw_players;

template <typename T>
void pack_msg(std::string& res, uint msg, T& stru)
{
	auto old_size = (uint)res.size();
	res.resize(old_size + sizeof(uint) + sizeof(T));
	auto dst = res.data() + old_size;
	memcpy(dst, &msg, sizeof(uint)); dst += sizeof(uint);
	memcpy(dst, &stru, sizeof(T));
}

void start_server();
void join_server();
