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
		AABB _b(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f));
		mat4 m(1.f);
		m = translate(mat4(1.f), node->global_pos());
		m = m * mat4(node->g_qut);
		b.expand(AABB(_b.get_points(m)));
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
