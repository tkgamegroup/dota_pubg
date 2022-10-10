#include "network.h"

MultiPlayerType multi_player = SinglePlayer;
network::ClientPtr nw_client = nullptr;
network::ServerPtr nw_server = nullptr;
std::map<uint, std::vector<void*>> nw_players;

PeedingActions<void*>					peeding_add_players;
PeedingActions<nwAddCharacterStruct>	peeding_add_characters;

void start_server()
{
	nw_server = network::Server::create(network::SocketTcp, 1234, nullptr, [](void* id) {
		nw_server->set_client(id, [](const std::string& msg) {

		},
		[]() {

		});

		peeding_add_players.mtx.lock();
		peeding_add_players.actions.push_back(id);
		peeding_add_players.mtx.unlock();
	});
	multi_player = MultiPlayerAsHost;
}

void join_server()
{
	nw_client = network::Client::create(network::SocketTcp, "127.0.0.1", 1234, [](const std::string& msg) {
		auto p = msg.data();
		auto e = p + msg.size();
		while (p < e)
		{
			auto msg = *(uint*)p;
			p += sizeof(uint);
			switch (msg)
			{
			case nwNewPlayerInfo:
			{
				auto& s = *(nwNewPlayerInfoStruct*)p;
				main_player.faction = s.faction;
				main_player.character_id = s.character_id;
				p += sizeof(nwNewPlayerInfoStruct);
			}
				break;
			case nwAddCharacter:
				peeding_add_characters.mtx.lock();
				peeding_add_characters.actions.push_back(*(nwAddCharacterStruct*)p);
				peeding_add_characters.mtx.unlock();
				p += sizeof(nwAddCharacterStruct);
				break;
			}
		}
	},
	[]() {

	});
}
