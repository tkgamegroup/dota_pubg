#include "buff.h"

std::vector<Buff> buffs;

int Buff::find(const std::string& name)
{
	for (auto i = 0; i < buffs.size(); i++)
	{
		if (buffs[i].name == name)
			return i;
	}
	return -1;
}

const Buff& Buff::get(uint id)
{
	assert(id < buffs.size());
	return buffs[id];
}

void load_buffs()
{
	{
		auto& buff = buffs.emplace_back();

	}
}
