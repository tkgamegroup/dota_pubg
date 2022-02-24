#include <flame/universe/component.h>

using namespace flame;

struct cMain : Component
{
	void start() override
	{
		printf("Hello World\n");
	}
};
