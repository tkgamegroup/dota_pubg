#include <flame/universe/components/particle_system.h>
#include <flame/universe/components/audio_source.h>

#include "effect.h"
#include "object.h"
#include "character.h"

std::vector<cEffectPtr> effects;
std::vector<cEffectPtr> dead_effects;
bool removing_dead_effects = false;

//LinkEffect::LinkEffect(cEffectPtr effect) :
//	effect(effect)
//{
//	rnd = rand();
//}
//
//LinkEffect::~LinkEffect()
//{
//	if (target0.obj)
//		target0.obj->node->data_listeners.remove(rnd);
//	if (target1.obj)
//		target1.obj->node->data_listeners.remove(rnd);
//}
//
//void LinkEffect::update()
//{
//	effect->node->set_pos((pos0 + pos1) * 0.5f);
//
//	if (auto ps = effect->particle_system; ps)
//	{
//		auto ptcs = ps->get_particles();
//		int n = ptcs.size();
//		auto ang = radians(angle_xz(pos1 - pos0));
//		auto len = distance(pos0, pos1) / n * 0.5f;
//		auto mat_inv = inverse(ps->node->transform);
//		for (auto i = 0; i < n; i++)
//		{
//			auto& ptc = ptcs[i];
//			ptc.pos = mat_inv * vec4(mix(pos0, pos1, float(i + 1) / float(n + 1)), 1.f);
//			ptc.size.x = len;
//			ptc.rot = ang;
//		}
//		ps->set_particles(ptcs);
//	}
//}
//
//void LinkEffect::send_message(uint hash, void* data, uint size)
//{
//	switch (hash)
//	{
//	case "Target0"_h:
//		if (size == sizeof(IDAndPos))
//		{
//			auto& iap = *(IDAndPos*)data;
//			if (auto it = objects.find(iap.id); it != objects.end())
//			{
//				auto character = it->second->entity->get_component<cCharacter>();
//				target0.set(character);
//				auto node = character->node;
//				node->data_listeners.add([this, node](uint hash) {
//					if (hash == "pos"_h)
//						pos0 = node->pos;
//				}, rnd);
//			}
//			pos0 = iap.pos;
//		}
//		break;
//	case "Target1"_h:
//		if (size == sizeof(IDAndPos))
//		{
//			auto& iap = *(IDAndPos*)data;
//			if (auto it = objects.find(iap.id); it != objects.end())
//			{
//				auto character = it->second->entity->get_component<cCharacter>();
//				target1.set(character);
//				auto node = character->node;
//				node->data_listeners.add([this, node](uint hash) {
//					if (hash == "pos"_h)
//						pos1 = node->pos;
//				}, rnd);
//			}
//			pos1 = iap.pos;
//		}
//		break;
//	}
//}

cEffect::~cEffect()
{
	std::erase_if(effects, [this](const auto& i) {
		return i == this;
	});
	if (dead && !removing_dead_effects)
	{
		std::erase_if(dead_effects, [this](const auto& i) {
			return i == this;
		});
	}
}

void cEffect::set_type(uint t)
{
	if (type == t)
		return;
	type = t;

	//switch (type)
	//{
	//case "Normal"_h:
	//	special_effect.reset(nullptr);
	//	break;
	//case "Link"_h:
	//	special_effect.reset(new LinkEffect(this));
	//	break;
	//}
}

void cEffect::start()
{
	particle_system = entity->get_component<cParticleSystem>();
	if (!particle_system && !entity->children.empty())
		particle_system = entity->children[0]->get_component<cParticleSystem>();

	timer = duration;

	std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
	if (!sound_path.empty())
		audio_buffer_names.emplace_back(sound_path, "start");
	audio_source->set_buffer_names(audio_buffer_names);
	audio_source->play("start"_h);
}

void cEffect::update()
{
	if (dead)
		return;

	//if (special_effect)
	//	special_effect->update();

	if (timer > 0.f)
	{
		timer -= delta_time;
		if (timer < 0.f)
			die();
	}
}

void cEffect::send_message(uint hash, void* data, uint size)
{
	//if (special_effect)
	//	special_effect->send_message(hash, data, size);
}

void cEffect::die()
{
	if (dead)
		return;

	dead_effects.push_back(this);
	dead = true;
}

struct cEffectCreate : cEffect::Create
{
	cEffectPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cEffect;
	}
}cEffect_create;
cEffect::Create& cEffect::create = cEffect_create;
