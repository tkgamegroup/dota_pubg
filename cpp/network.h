#pragma once

#include "main.h"

#include <flame/foundation/network.h>
#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>

enum MultiPlayerType
{
	SinglePlayer,
	MultiPlayerAsHost,
	MultiPlayerAsClient
};

enum nwMessage
{
	nwNewPlayerInfo,
	nwAddObjects,
	nwRemoveObjects,
	nwUpdateObjects,
	nwCommandCharacter
};

struct nwNewPlayerInfoStruct
{
	uint faction;
	uint character_id;
};

struct nwAddObjectsStruct
{
	struct Item
	{
		uint preset_id;
		uint id;
	};

	std::vector<Item> items;
};

struct nwRemoveObjectsStruct
{
	std::vector<uint> ids;
};

struct nwUpdateObjectsStruct
{
	struct Comp
	{
		uchar idx;
		std::vector<uint> names;
		std::string datas;
	};

	struct Item
	{
		uint obj_id;
		std::vector<Comp> comps;
	};

	std::vector<Item> items;
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

inline void pack_msg(std::ostringstream& res, uint len, char* data)
{
	res << std::string_view(data, len);
}

template <typename T>
inline void pack_msg(std::ostringstream& res, uint msg, T& stru)
{
	res << (char)msg;
	serialize_binary(&stru, res);
}

void start_server();
void join_server();
