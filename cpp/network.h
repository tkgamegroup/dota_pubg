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
	nwUpdateCharacter
};

struct nwNewPlayerInfoStruct
{
	uint faction;
	uint character_id;
};

struct nwAddCharacterStruct
{
	wchar_t path[256];
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
	vec3 euler;
	uint action;
};

template <typename T>
struct PeedingActions
{
	std::mutex mtx;
	std::vector<T> actions;
};

extern PeedingActions<void*>				peeding_add_players;
extern PeedingActions<nwAddCharacterStruct> peeding_add_characters;

extern MultiPlayerType multi_player;
extern network::ClientPtr nw_client;
extern network::ServerPtr nw_server;

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
