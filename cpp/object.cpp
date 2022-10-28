#include "object.h"
#include "network.h"

static uint g_uid = 1;
std::map<uint, cObjectPtr> objects;

cObject::~cObject()
{
	if (auto it = objects.find(uid); it != objects.end())
		objects.erase(it);

	if (multi_player == MultiPlayerAsHost)
	{
		for (auto& f : nw_players)
		{
			std::string res;
			pack_msg(res, nwRemoveObject, uid);
			for (auto so_id : f.second)
				so_server->send(so_id, res);
		}
	}
}

void cObject::set_visible_flags(uint v)
{
	if (visible_flags == v)
		return;
	visible_flags = v;
	data_changed("visible_flags"_h);
}

void cObject::set_uid(uint id)
{
	uid = id ? id : g_uid++;
	objects[id] = this;
}

struct cObjectCreate : cObject::Create
{
	cObjectPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cObject;
	}
}cObject_create;
cObject::Create& cObject::create = cObject_create;
