#include <flame/universe/entity.h>

#include "main.h"

void cMain::start()
{
	printf("Hello World\n");
}

struct cMainCreate : cMain::Create
{
	cMainPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cMain;
	}
}cMain_create;
cMain::Create& cMain::create = cMain_create;

EXPORT void* cpp_info()
{
	auto uinfo = universe_info();
	cMain::create((EntityPtr)INVALID_POINTER);
	return nullptr;
}
