#include "network.h"

std::mutex nw_mtx;
MultiPlayerType multi_player = SinglePlayer;
network::ClientPtr so_client = nullptr;
network::ServerPtr so_server = nullptr;
std::map<uint, std::vector<void*>> nw_players;
std::vector<void*> nw_new_players;
std::string nw_msgs;

void start_server()
{
	so_server = network::Server::create(network::SocketTcp, 1234, nullptr, [](void* id) {
		so_server->set_client(id, [](const std::string& msg) {
			nw_mtx.lock();
			nw_msgs += msg;
			nw_mtx.unlock();
		},
		[]() {

		});

		nw_mtx.lock();
		nw_new_players.push_back(id);
		nw_mtx.unlock();
	});
	multi_player = MultiPlayerAsHost;
}

void join_server()
{
	so_client = network::Client::create(network::SocketTcp, "127.0.0.1", 1234, [](const std::string& msg) {
		nw_mtx.lock();
		nw_msgs += msg;
		nw_mtx.unlock();
	},
	[]() {

	});
}
