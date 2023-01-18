#include "effect.h"
#include "character.h"

#include <flame/universe/components/node.h>
#include <flame/universe/components/particle_system.h>
#include <flame/universe/components/audio_source.h>

std::vector<EffectPreset> effect_presets;
static EffectPreset dummy_preset;

void init_effects()
{
	for (auto& section : parse_ini_file(Path::get(L"assets\\effects.ini")).sections)
	{
		auto& preset = effect_presets.emplace_back();
		preset.id = effect_presets.size() - 1;
		preset.name = section.name;
		for (auto& e : section.entries)
		{
			switch (e.key_hash)
			{
			case "path"_h:
				preset.path = e.values[0];
				break;
			case "sound_path"_h:
				preset.sound_path = e.values[0];
				break;
			}
		}
	}
}

int EffectPreset::find(const std::string& name)
{
	for (auto i = 0; i < effect_presets.size(); i++)
	{
		if (effect_presets[i].name == name)
			return i;
	}
	return -1;
}

const EffectPreset& EffectPreset::get(uint id)
{
	return effect_presets[id];
}

std::vector<cEffectPtr> effects;
std::vector<cEffectPtr> dead_effects;
bool removing_dead_effects = false;

LinkEffect::LinkEffect(cEffectPtr effect) :
	effect(effect)
{
	rnd = rand();
}

LinkEffect::~LinkEffect()
{
	if (target0.obj)
		target0.obj->node->data_listeners.remove(rnd);
	if (target1.obj)
		target1.obj->node->data_listeners.remove(rnd);
}

void LinkEffect::update()
{
	effect->node->set_pos((pos0 + pos1) * 0.5f);

	if (auto ps = effect->particle_system; ps)
	{
		auto ptcs = ps->get_particles();
		int n = ptcs.size();
		auto ang = radians(angle_xz(pos1 - pos0));
		auto len = distance(pos0, pos1) / n * 0.5f;
		auto mat_inv = inverse(ps->node->transform);
		for (auto i = 0; i < n; i++)
		{
			auto& ptc = ptcs[i];
			ptc.pos = mat_inv * vec4(mix(pos0, pos1, float(i + 1) / float(n + 1)), 1.f);
			ptc.size.x = len;
			ptc.rot = ang;
		}
		ps->set_particles(ptcs);
	}
}

void LinkEffect::send_message(uint hash, void* data, uint size)
{
	switch (hash)
	{
	case "Target0"_h:
		if (size >= sizeof(void*))
		{
			auto character = *(cCharacterPtr*)data;
			target0.set(character);
			auto node = character->node;
			pos0 = node->pos;
			node->data_listeners.add([this, node](uint hash) {
				if (hash == "pos"_h)
					pos0 = node->pos;
			}, rnd);
		}
		break;
	case "Target1"_h:
		if (size >= sizeof(void*))
		{
			auto character = *(cCharacterPtr*)data;
			target1.set(character);
			auto node = character->node;
			pos1 = node->pos;
			node->data_listeners.add([this, node](uint hash) {
				if (hash == "pos"_h)
					pos1 = node->pos;
			}, rnd);
		}
		break;
	}
}

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

	switch (type)
	{
	case "Normal"_h:
		special_effect.reset(nullptr);
		break;
	case "Link"_h:
		special_effect.reset(new LinkEffect(this));
		break;
	}
}

void cEffect::start()
{
	particle_system = entity->get_component_t<cParticleSystem>();
	if (!particle_system && !entity->children.empty())
		particle_system = entity->children[0]->get_component_t<cParticleSystem>();

	timer = duration;

	if (!preset)
		preset = &dummy_preset;

	std::vector<std::pair<std::filesystem::path, std::string>> audio_buffer_names;
	if (!preset->sound_path.empty())
		audio_buffer_names.emplace_back(preset->sound_path, "start");
	audio_source->set_buffer_names(audio_buffer_names);
	audio_source->play("start"_h);
}

void cEffect::update()
{
	if (dead)
		return;

	if (special_effect)
		special_effect->update();

	if (timer > 0.f)
	{
		timer -= delta_time;
		if (timer < 0.f)
			die();
	}
}

void cEffect::send_message(uint hash, void* data, uint size)
{
	if (special_effect)
		special_effect->send_message(hash, data, size);
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
