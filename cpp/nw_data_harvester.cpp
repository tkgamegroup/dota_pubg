#include "nw_data_harvester.h"

#include <flame/foundation/typeinfo.h>

void cNWDataHarvester::add_target(uint comp, uint var)
{
	targets[comp][var] = 0xffffffff;
}

void cNWDataHarvester::start()
{
	for (auto& pair : targets)
	{
		auto comp = entity->get_component(pair.first);
		if (comp)
		{
			auto& map = pair.second;
			comp->data_listeners.add([&map](uint hash) {
				auto it = map.find(hash);
				if (it != map.end())
					it->second = 0xffffffff;
			});
		}
		else
			pair.second.clear();
	}
	for (auto it = targets.begin(); it != targets.end();)
	{
		if (it->second.empty())
			it = targets.erase(it);
		else
			it++;
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
