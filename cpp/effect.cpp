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
}

LinkEffect::~LinkEffect()
{
	if (target_character.obj)
		target_character.obj->node->data_listeners.remove("link_effect"_h);
}

void LinkEffect::init(void* data, uint size)
{
	if (size >= sizeof(void*))
	{
		target_character.set(*(cCharacterPtr*)data);
		auto node = target_character.obj->node;
		target_pos = node->pos;
		node->data_listeners.add([this, node](uint hash) {
			if (hash == "pos"_h)
				target_pos = node->pos;
		}, "link_effect"_h);
	}
}

void LinkEffect::update()
{
	if (auto ps = effect->particle_system; ps)
	{
		auto ptcs = ps->get_particles();
		int n = ptcs.size();
		auto p0 = ps->node->g_pos;
		auto p1 = target_pos;
		auto ang = radians(angle_xz(p1 - p0));
		auto len = distance(p0, p1) / n * 0.5f;
		auto mat_inv = inverse(ps->node->transform);
		for (auto i = 0; i < n; i++)
		{
			auto& ptc = ptcs[i];
			ptc.pos = mat_inv * vec4(mix(p0, p1, float(i + 1) / float(n + 1)), 1.f);
			ptc.size.x = len;
			ptc.rot = ang;
		}
		ps->set_particles(ptcs);
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
