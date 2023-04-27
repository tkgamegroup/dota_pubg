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
		AABB _b(vec3(-0.3f, 0.f, -0.2f), vec3(0.3f, 0.4f, 0.2f));
		mat4 m(1.f);
		m = translate(mat4(1.f), node->global_pos());
		m = m * mat4(node->g_qut);
		b.expand(AABB(_b.get_points(m)));
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
