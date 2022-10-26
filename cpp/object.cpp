#include "object.h"
#include "network.h"

static uint uid = 1;
std::map<uint, cObjectPtr> objects;

cObject::~cObject()
{
	objects.erase(objects.find(uid));

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

void add_object(uint id)
{
	id = id ? id : uid++;

	//objects[id] = object;
}
