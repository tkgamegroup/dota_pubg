#include "nw_data_harvester.h"

#include <flame/foundation/typeinfo.h>

void cNWDataHarvester::add_target(uint var, uint flags)
{
	targets[var] = std::make_pair(flags, flags);
}

void cNWDataHarvester::on_init()
{
	for (auto& comp : entity->components)
	{
		comp->data_listeners.add([this](uint hash) {
			auto it = targets.find(hash);
			if (it != targets.end())
				it->second.first = it->second.second;
		});
	}
}

struct cNWDataHarvesterCreate : cNWDataHarvester::Create
{
	cNWDataHarvesterPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cNWDataHarvester;
	}
}cNWDataHarvester_create;
cNWDataHarvester::Create& cNWDataHarvester::create = cNWDataHarvester_create;
