#include <flame/universe/application.h>

using namespace flame;

UniverseApplication app;

int main()
{
	Path::set_root(L"assets", std::filesystem::current_path() / L"assets");
	app.create("dota_pubg", uvec2(1280, 720), WindowFrame | WindowResizable, false, true, { { "mesh_shader"_h, 0 } });
	app.world->root->load(L"assets/launcher.prefab");
	app.renderer->bind_window_targets();
	app.renderer->mode = sRenderer::CameraLight;
	graphics::gui_set_clear(true, vec4(0.f));
	app.run();
	return 0;
}
