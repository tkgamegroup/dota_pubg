#include "object.h"

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
