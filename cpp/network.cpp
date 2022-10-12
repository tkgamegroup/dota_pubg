#include "network.h"

MultiPlayerType multi_player = SinglePlayer;
network::ClientPtr so_client = nullptr;
network::ServerPtr so_server = nullptr;
std::map<uint, std::vector<void*>> nw_players;

PeedingActions<void*>											peeding_add_players;
PeedingActions<nwAddCharacterStruct>							peeding_add_characters;
PeedingActions<nwRemoveCharacterStruct>							peeding_remove_characters;
PeedingActions<std::pair<nwUpdateCharacterStruct, std::string>>	peeding_update_characters;
PeedingActions<nwCommandCharacterStruct>						peeding_command_characters;
PeedingActions<nwAddProjectileStruct>							peeding_add_projectiles;
PeedingActions<nwRemoveProjectileStruct>						peeding_remove_projectiles;
PeedingActions<nwAddChestStruct>								peeding_add_chests;
PeedingActions<nwRemoveChestStruct>								peeding_remove_chests;

void start_server()
{
	so_server = network::Server::create(network::SocketTcp, 1234, nullptr, [](void* id) {
		so_server->set_client(id, [](const std::string& msg) {
			auto p = msg.data();
			auto e = p + msg.size();
			while (p < e)
			{
				auto msg = *(uchar*)p;
				p += sizeof(uchar);
				switch (msg)
				{
				case nwCommandCharacter:
					peeding_command_characters.mtx.lock();
					peeding_command_characters.actions.push_back(*(nwCommandCharacterStruct*)p);
					peeding_command_characters.mtx.unlock();
					p += sizeof(nwCommandCharacterStruct);
					break;
				}
			}
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
	so_client = network::Client::create(network::SocketTcp, "127.0.0.1", 1234, [](const std::string& msg) {
		auto p = msg.data();
		auto e = p + msg.size();
		while (p < e)
		{
			auto msg = *(uchar*)p;
			p += sizeof(uchar);
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
			case nwRemoveCharacter:
				peeding_remove_characters.mtx.lock();
				peeding_remove_characters.actions.push_back(*(nwRemoveCharacterStruct*)p);
				peeding_remove_characters.mtx.unlock();
				p += sizeof(nwRemoveCharacterStruct);
				break;
			case nwUpdateCharacter:
			{
				auto& stru = *(nwUpdateCharacterStruct*)p;
				p += sizeof(nwUpdateCharacterStruct);
				std::string extra;
				if (stru.extra_length > 0)
				{
					extra = std::string(p, p + stru.extra_length);
					p += stru.extra_length;
				}
				peeding_update_characters.mtx.lock();
				peeding_update_characters.actions.emplace_back(stru, extra);
				peeding_update_characters.mtx.unlock();
			}
				break;
			case nwAddProjectile:
				peeding_add_projectiles.mtx.lock();
				peeding_add_projectiles.actions.push_back(*(nwAddProjectileStruct*)p);
				peeding_add_projectiles.mtx.unlock();
				p += sizeof(nwAddProjectileStruct);
				break;
			case nwRemoveProjectile:
				peeding_remove_projectiles.mtx.lock();
				peeding_remove_projectiles.actions.push_back(*(nwRemoveProjectileStruct*)p);
				peeding_remove_projectiles.mtx.unlock();
				p += sizeof(nwRemoveProjectileStruct);
				break;
			case nwAddChest:
				peeding_add_chests.mtx.lock();
				peeding_add_chests.actions.push_back(*(nwAddChestStruct*)p);
				peeding_add_chests.mtx.unlock();
				p += sizeof(nwAddChestStruct);
				break;
			case nwRemoveChest:
				peeding_remove_chests.mtx.lock();
				peeding_remove_chests.actions.push_back(*(nwRemoveChestStruct*)p);
				peeding_remove_chests.mtx.unlock();
				p += sizeof(nwRemoveProjectileStruct);
				break;
			}
		}
	},
	[]() {

	});
}
