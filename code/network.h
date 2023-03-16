#pragma once

#include <flame/foundation/network.h>
#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>

#include "head.h"

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
		uint prefab_id;
		uint id;
	};

	// Reflect length_bytes=2
	std::vector<Item> items;
};

struct nwRemoveObjectsStruct
{
	// Reflect length_bytes=2
	std::vector<uint> ids;
};

struct nwUpdateObjectsStruct
{
	struct Comp
	{
		uchar idx;
		// Reflect length_bytes=1
		std::vector<uint> names;
		std::string datas;
	};

	struct Item
	{
		uint obj_id;
		// Reflect length_bytes=1
		std::vector<Comp> comps;
	};

	// Reflect length_bytes=2
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

// Reflect ctor
struct cNWDataHarvester : Component
{
	std::vector<std::unordered_map<uint/*var hash*/, std::pair<uint, uint>/*the current and reset faction flags (is this var needs to sync to those factions)*/>> targets;

	void add_target(uint comp, uint var, uint flags = 0xffffffff);

	void on_init() override;

	struct Create
	{
		virtual cNWDataHarvesterPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};

extern std::mutex nw_mtx;
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
