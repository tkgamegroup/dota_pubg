#include "town.h"

struct cTownCreate : cTown::Create
{
	cTownPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cTown;
	}
}cTown_create;
cTown::Create& cTown::create = cTown_create;
