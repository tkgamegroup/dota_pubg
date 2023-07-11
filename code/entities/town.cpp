#include "town.h"

std::vector<cTownPtr> towns;

cTown::~cTown()
{
	std::erase_if(towns, [this](const auto& i) {
		return i == this;
	});
}

void cTown::on_init()
{
}

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
