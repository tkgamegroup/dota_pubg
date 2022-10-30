#include "object.h"
#include "network.h"

static uint g_uid = 1;
std::map<uint, cObjectPtr> objects;
std::vector<std::pair<uint, uint>>	new_objects;
std::vector<uint>					removed_objects;

cObject::~cObject()
{
	if (auto it = objects.find(uid); it != objects.end())
		objects.erase(it);

	if (multi_player == MultiPlayerAsHost)
		removed_objects.push_back(uid);
}

void cObject::set_visible_flags(uint v)
{
	if (visible_flags == v)
		return;
	visible_flags = v;
	data_changed("visible_flags"_h);
}

void cObject::init(uint _preset_id, uint _uid)
{
	preset_id = _preset_id;
	uid = _uid ? _uid : g_uid++;
	objects[uid] = this;

	if (multi_player == MultiPlayerAsHost)
		new_objects.emplace_back(preset_id, uid);
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
