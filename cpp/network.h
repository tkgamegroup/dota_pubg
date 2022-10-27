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
	nwAddObject,
	nwRemoveObject,
	nwUpdateObject,
	nwCommandCharacter
};

struct nwNewPlayerInfoStruct
{
	uint faction;
	uint character_id;
};

struct nwAddObjectStruct
{
	uint preset_id;
	uint id;
};

struct nwCommandCharacterStruct
{
	uint id;
	uint type;
	uint id2;
	union 
	{
		vec3 location;
		uint target;
	}t;
};

extern std::mutex nw_mtx;
extern MultiPlayerType multi_player;
extern network::ClientPtr so_client;
extern network::ServerPtr so_server;
extern std::map<uint, std::vector<void*>> nw_players;
extern std::vector<void*> nw_new_players;
extern std::string nw_msgs;

inline void pack_msg(std::string& res, uint len, char* data)
{
	auto old_size = (uint)res.size();
	res.resize(old_size + len);
	auto dst = res.data() + old_size;
	memcpy(dst, data, len);
}

template <typename T>
inline void pack_msg(std::string& res, uint msg, T& stru)
{
	auto old_size = (uint)res.size();
	res.resize(old_size + sizeof(uchar) + sizeof(T));
	auto dst = res.data() + old_size;
	memcpy(dst, &msg, sizeof(uchar)); dst += sizeof(uchar);
	if (std::is_pod_v<T>)
		memcpy(dst, &stru, sizeof(T));
	else
		assert(0);
}

void start_server();
void join_server();
