#include "nw_data_harvester.h"

#include <flame/foundation/typeinfo.h>

void cNWDataHarvester::add_target(uint var)
{
	targets[var] = 0xffffffff;
}

void cNWDataHarvester::on_init()
{
	auto comp = entity->get_component("cCharacter"_h);
	  
	comp->data_listeners.add([this](uint hash) {
		auto it = targets.find(hash);
		if (it != targets.end())
			it->second = 0xffffffff;
	});
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
