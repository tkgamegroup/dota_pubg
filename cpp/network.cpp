#include "network.h"

void cNWDataHarvester::add_target(uint comp, uint var, uint flags)
{
	auto idx = 0;
	for (; idx < entity->components.size() - 1; idx++)
	{
		if (entity->components[idx]->type_hash == comp)
			break;
	}
	if (idx == entity->components.size() - 1)
		return;
	targets[idx][var] = std::make_pair(flags, flags);
}

void cNWDataHarvester::on_init()
{
	targets.resize(entity->components.size());
	for (auto idx = 0; idx < entity->components.size(); idx++)
	{
		entity->components[idx]->data_listeners.add([this, idx](uint hash) {
			auto it = targets[idx].find(hash);
			if (it != targets[idx].end())
				it->second.first = it->second.second;
			});
	}
	add_target("cObject"_h, "visible_flags"_h);
}

struct cNWDataHarvesterCreate : cNWDataHarvester::Create
{
	cNWDataHarvesterPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cNWDataHarvester;
	}
}cNWDataHarvester_create;
cNWDataHarvester::Create& cNWDataHarvester::create = cNWDataHarvester_create;

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
