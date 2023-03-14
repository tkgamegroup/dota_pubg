#include "chest.h"

std::vector<cChestPtr> chests;
std::vector<cChestPtr> dead_chests;
bool removing_dead_chests = false;

cChest::~cChest()
{
	std::erase_if(chests, [this](const auto& i) {
		return i == this;
	});
	if (dead && !removing_dead_chests)
	{
		std::erase_if(dead_chests, [this](const auto& i) {
			return i == this;
		});
	}
}
void cChest::on_init()
{
	node->measurers.add([this](AABB& b) {
		b.expand(AABB(AABB(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f)).get_points(node->transform)));
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
