#include "command.h"
#include "character.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>

Variant read_variant(std::string& str)
{
	Variant ret;
	switch (str.back())
	{
	case 'f':
		str.pop_back();
		ret.f = s2t<float>(str);
		break;
	case 'u':
		str.pop_back();
		ret.f = s2t<uint>(str);
		break;
	case 'i':
		str.pop_back();
	default:
		ret.i = s2t<int>(str);
	}
	return ret;
}

void read_parameter_values(std::vector<Variant>& vec, const std::string& text)
{
	auto sp = SUS::split(text, '/');
	vec.resize(sp.size());
	for (auto i = 0; i < vec.size(); i++)
		vec[i] = read_variant(sp[i]);
}

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::string& text)
{
	for (auto& p : SUS::split(text))
	{
		auto sp = SUS::split(p, ':');
		auto hash = sh(sp[0].c_str());
		parameter_names.emplace_back(sp[0], hash);
		auto& vec = parameters.emplace(hash, std::vector<Variant>()).first->second;
		read_parameter_values(vec, sp[1]);
	}
}

Command::Command(std::vector<std::string>& tokens)
{
	static auto ei_type = find_enum(th<Type>());

	type = (Type)ei_type->find_item(tokens[0])->value;
	parameters.resize(tokens.size() - 1);
	for (auto i = 1; i < tokens.size(); i++)
	{
		auto& para = parameters[i - 1];
		auto name = 0;
		auto& str = tokens[i];
		if (std::isdigit(str[0]))
			para = { 0, read_variant(str) };
		else
		{
			if (auto ii = ei_type->find_item(str); ii)
				para = { 0, { .i = ii->value } };
			else if (SUS::strip_head_tail_if(str, "\"", "\"_buff"))
				para = { 0, { .i = Buff::find(str) } };
			else
				para = { 1, { .u = sh(str.c_str()) } };
		}
	}
}

void Command::execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint idx, uint lv)->const Variant& {
		auto& para = parameters[idx];
		if (para.first == 0)
			return para.second;
		auto& vec = external_parameters.at(para.second.u);
		return lv <= vec.size() ? vec[lv] : vec[0];
	};

	auto off = 0;

	std::function<void(Type type)> do_cmd;
	do_cmd = [&](Type type) {
		switch (type)
		{
		case tRollDice100:
			if (linearRand(0U, 99U) < read_parameter(off, lv).i)
			{
				type = (Type)parameters[off + 1].second.i;
				off += 2;
				do_cmd(type);
			}
			break;
		case tRestoreHP:
			character->restore_hp(read_parameter(off, lv).i);
			break;
		case tRestoreMP:
			character->restore_mp(read_parameter(off, lv).i);
			break;
		case tTakeDamage:
			break;
		case tTakeDamagePct:
			character->take_damage(MagicDamage, character->hp_max * (read_parameter(off, lv).i / 100.f));
			break;
		case tLevelUp:
			character->gain_exp(character->exp_max);
			break;
		case tIncreaseHPMax:
			character->hp_max += read_parameter(off, lv).i;
			break;
		case tIncreaseMPMax:
			character->mp_max += read_parameter(off, lv).i;
			break;
		case tIncreaseATK:
			character->atk += read_parameter(off, lv).i;
			break;
		case tIncreasePHYDEF:
			character->phy_def += read_parameter(off, lv).i;
			break;
		case tIncreaseMAGDEF:
			character->mag_def += read_parameter(off, lv).i;
			break;
		case tIncreaseHPREG:
			character->hp_reg += read_parameter(off, lv).i;
			break;
		case tIncreaseMPREG:
			character->mp_reg += read_parameter(off, lv).i;
			break;
		case tIncreaseMOVSP:
			character->mov_sp += read_parameter(off, lv).i;
			break;
		case tIncreaseATKSP:
			character->atk_sp += read_parameter(off, lv).i;
			break;
		case tIncreaseHPMaxPct:
			character->hp_max *= (100 + read_parameter(off, lv).i) / 100.f;
			break;
		case tIncreaseMPMaxPct:
			break;
		case tIncreaseATKPct:
			character->atk *= (100 + read_parameter(off, lv).i) / 100.f;
			break;
		case tAddBuff:
			break;
		case tAddBuffToTarget:
			target_character->add_buff(read_parameter(off, lv).i, read_parameter(off + 1, lv).f, read_parameter(off + 2, lv).u, read_parameter(off + 3, lv).u);
			break;
		case tAddAttackEffect:
		{
			CommandList ef;
			Command cmd;
			cmd.type = (Type)parameters[off].second.i;
			cmd.parameters.resize(parameters.size() - off - 1);
			for (auto i = 0; i < cmd.parameters.size(); i++)
				cmd.parameters[i] = parameters[i + off + 1];
			ef.push_back(cmd);
			character->attack_effects.push_back(ef);
		}
			break;
		}
	};

	do_cmd(type);
}

void parse_command_list(CommandList& list, const std::string& text)
{
	for (auto& c : SUS::split(text, '&'))
	{
		auto tokens = SUS::split_quot(SUS::get_trimed(c));
		list.push_back(Command(tokens));
	}
}
