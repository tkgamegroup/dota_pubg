#include "launcher.h"
#include "network.h"

#include <flame/graphics/gui.h>

cLauncher::~cLauncher()
{
	graphics::gui_callbacks.remove((uint)this);
}

void enter_scene(EntityPtr root)
{
	add_event([root]() {
		root->remove_component<cLauncher>();
		root->load(L"assets\\main.prefab");
		return false;
	});
}

void cLauncher::start()
{
	graphics::gui_set_current();
	graphics::gui_callbacks.add([this]() {
		if (ImGui::Button("Single Player"))
		{
			enter_scene(entity);
		}
		if (ImGui::Button("Create Local Server"))
		{
			start_server();
			enter_scene(entity);
		}
		if (ImGui::Button("Join Local Server"))
		{
			join_server();
			if (nw_client)
			{
				multi_player = MultiPlayerAsClient;
				enter_scene(entity);
			}
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