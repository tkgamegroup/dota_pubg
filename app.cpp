#include <flame/universe/application.h>

using namespace flame;

UniverseApplication app;

extern "C" IMPORT void* cpp_info();

int main()
{
	auto info = cpp_info();
	Path::set_root(L"assets", std::filesystem::current_path() / L"assets");
	app.create("dota_pubg", uvec2(1280, 720), WindowFrame | WindowResizable, true);
	app.world->root->load(L"assets/launcher.prefab");
	app.renderer->bind_window_targets();
	app.run();
	return 0;
}
