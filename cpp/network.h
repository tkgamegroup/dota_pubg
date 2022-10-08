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
	nwAddCharacter,
	nwRemoveCharacter,
	nwUpdateCharacter
};

struct nwAddCharacterStruct
{
	char path[256];
	char guid[32];
	vec3 pos;
};

struct nwRemoveCharacterStruct
{

};

struct nwUpdateCharacterStruct
{

};

extern MultiPlayerType multi_player;
extern network::ClientPtr nw_client;
extern network::ServerPtr nw_server;

void start_server();
void join_server();
