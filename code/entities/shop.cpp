#include "shop.h"

std::vector<cShopPtr> shops;
std::vector<cShopPtr> dead_shops;
bool removing_dead_shops = false;

cShop::~cShop()
{
	std::erase_if(shops, [this](const auto& i) {
		return i == this;
	});
	if (dead && !removing_dead_shops)
	{
		std::erase_if(dead_shops, [this](const auto& i) {
			return i == this;
		});
	}
}
void cShop::on_init()
{
	node->measurers.add([this](AABB& b) {
		b.expand(AABB(AABB(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f)).get_points(node->transform)));
	}, "shop"_h);
}

void cShop::die()
{
	if (dead)
		return;

	dead_shops.push_back(this);
	dead = true;
}

struct cShopCreate : cShop::Create
{
	cShopPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cShop;
	}
}cShop_create;
cShop::Create& cShop::create = cShop_create;