#include "chest.h"

#include <flame/universe/components/node.h>

std::vector<cChestPtr> chests;
std::vector<cChestPtr> dead_chests;

cChest::~cChest()
{
	std::erase_if(chests, [this](const auto& i) {
		return i == this;
	});
}
void cChest::on_init()
{
	node->measurers.add([this](AABB* ret) {
		*ret = AABB(AABB(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f)).get_points(node->transform));
		return true;
	}, "chest"_h);
}

void cChest::die()
{
	if (dead)
		return;

	dead_chests.push_back(this);
	dead = true;
}

struct cChestCreate : cChest::Create
{
	cChestPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cChest;
	}
}cChest_create;
cChest::Create& cChest::create = cChest_create;
