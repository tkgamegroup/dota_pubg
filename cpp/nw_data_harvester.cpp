#include "nw_data_harvester.h"

#include <flame/foundation/typeinfo.h>

void cNWDataHarvester::add_target(uint var, uint flags)
{
	targets[var] = std::make_pair(flags, flags);
}

void cNWDataHarvester::process_data_changed(uint hash)
{
	auto it = targets.find(hash);
	if (it != targets.end())
		it->second.first = it->second.second;
}

void cNWDataHarvester::on_init()
{
	// listen to the node for pos, eul, etc.
	entity->components.front()->data_listeners.add([this](uint hash) {
		process_data_changed(hash);
	});
	// listen to object component
	entity->get_component(th<cObject>())->data_listeners.add([this](uint hash) {
		process_data_changed(hash);
	});
	// listen to the last component, such as character, projectile, chest
	entity->components.back()->data_listeners.add([this](uint hash) {
		process_data_changed(hash);
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
