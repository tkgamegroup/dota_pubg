#include "nw_data_harvester.h"

#include <flame/foundation/typeinfo.h>

void cNWDataHarvester::add_target(uint comp, uint var, uint flags)
{
	auto idx = 0;
	for (; idx < entity->components.size() - 1; idx++)
	{
		if (entity->components[idx]->type_hash == comp)
			break;
	}
	if (idx == entity->components.size() - 1)
		return;
	targets[idx][var] = std::make_pair(flags, flags);
}

void cNWDataHarvester::on_init()
{
	targets.resize(entity->components.size());
	for (auto idx = 0; idx < entity->components.size(); idx++)
	{
		entity->components[idx]->data_listeners.add([this, idx](uint hash) {
			auto it = targets[idx].find(hash);
			if (it != targets[idx].end())
				it->second.first = it->second.second;
		});
	}
	add_target("cObject"_h, "visible_flags"_h);
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
