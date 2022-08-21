#include "chest.h"

#include <flame/universe/components/node.h>

void cChest::on_init()
{
	node->measurers.add([this](AABB* ret) {
		*ret = AABB(AABB(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f)).get_points(node->transform));
		return true;
	}, "chest"_h);
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
