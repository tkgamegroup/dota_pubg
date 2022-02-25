#include <flame/universe/application.h>

using namespace flame;

UniverseApplication app;

int main()
{
	app.create(false, "dota_pubg", uvec2(1280, 720), WindowFrame | WindowResizable);
	app.run();
	return 0;
}
