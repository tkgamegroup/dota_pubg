#include <flame/graphics/gui.h>

#include "launcher.h"

cLauncher::~cLauncher()
{
	graphics::gui_callbacks.remove((uint)this);
}

void cLauncher::start()
{
	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		if (ImGui::Button("Single Player"))
		{
			add_event([this]() {
				auto root = entity;
				root->remove_component<cLauncher>();
				root->load(L"assets\\main.prefab");
				return false;
			});
		}
		if (ImGui::Button("Create Local Server"))
		{
			//		nw_server = network::Server::create(network::SocketTcp, 1234, nullptr, [](void* id) {
			//			nw_server->set_client(id, [](const std::string& msg) {
			//
			//				},
			//				[]() {
			//
			//				});
			//			nw_server->send(id, "hello");
			//			});
			//		multi_player = MultiPlayerAsHost;
		}
		if (ImGui::Button("Join Local Server"))
		{
			//		nw_client = network::Client::create(network::SocketTcp, "127.0.0.1", 1234, [](const std::string& msg) {
			//			printf(msg.c_str());
			//			},
			//			[]() {
			//
			//			});
			//		if (nw_client)
			//			;
		}
	}, (uint)this);
}

struct cLauncherCreate : cLauncher::Create
{
	cLauncherPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cLauncher;
	}
}cLauncher_create;
cLauncher::Create& cLauncher::create = cLauncher_create;
