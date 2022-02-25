#include <flame/universe/component.h>

using namespace flame;

FLAME_TYPE(cMain)

struct cMain : Component
{
	void start() override
	{
		printf("Hello World\n");
	}

	struct Create
	{
		virtual cMainPtr operator()(EntityPtr) = 0;
	};
	/// Reflect static
	__declspec(dllexport) static Create& create;
};

__declspec(dllexport)
void* cpp_info()
{
	return nullptr;
}
