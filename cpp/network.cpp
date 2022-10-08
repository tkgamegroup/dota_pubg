#include "network.h"

MultiPlayerType multi_player = SinglePlayer;
network::ClientPtr nw_client = nullptr;
network::ServerPtr nw_server = nullptr;

void start_server()
{
	nw_server = network::Server::create(network::SocketTcp, 1234, nullptr, [](void* id) {
		nw_server->set_client(id, [](const std::string& msg) {

		},
		[]() {

		});
	});
	multi_player = MultiPlayerAsHost;
}

void join_server()
{
	nw_client = network::Client::create(network::SocketTcp, "127.0.0.1", 1234, [](const std::string& msg) {
		auto p = msg.data();
		uint l = msg.size();
		while (l > 0)
		{
			auto msg = *(uint*)p;
			p += sizeof(uint);
			l -= 4;
			switch (msg)
			{
			case nwAddCharacter:

				break;
			}
		}
	},
	[]() {

	});
}
