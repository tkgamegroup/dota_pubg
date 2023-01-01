#include "command.h"
#include "character.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>

Variant read_variant(const std::string& str)
{
	Variant ret;
	switch (str.back())
	{
	case 'f':
		ret.f = s2t<float>(str);
		break;
	case 'u':
		ret.f = s2t<uint>(str);
		break;
	case 'i':
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

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::vector<std::string>& tokens)
{
	for (auto& t : tokens)
	{
		auto sp = SUS::split(t, ':');
		auto hash = sh(sp[0].c_str());
		parameter_names.emplace_back(sp[0], hash);
		auto& vec = parameters.emplace(hash, std::vector<Variant>()).first->second;
		read_parameter_values(vec, sp[1]);
	}
}

void CommandList::init_sub_groups()
{
	std::stack<uint> sg_stack;
	for (auto i = 0; i < cmds.size(); i++)
	{
		auto& c = cmds[i];
		auto type = data[c.first].i;
		if (type == tBeginSub)
			sg_stack.push(i);
		else if (type == tEndSub)
		{
			auto idx = sg_stack.top();
			sg_stack.pop();
			sub_groups.emplace(idx, i);
		}
	}
}

void CommandList::execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint idx, uint lv)->const Variant& {
		if (use_ext_para[idx] == 0)
			return data[idx];
		auto& vec = external_parameters.at(data[idx].u);
		return lv <= vec.size() ? vec[lv] : vec[0];
	};

	auto i = 0;
	std::function<void()> do_cmd;
	do_cmd = [&]() {
		auto& c = cmds[i];
		switch (data[c.first].i)
		{
		case tRollDice100:
		{
			auto pass = c.second >= 2 && linearRand(0U, 99U) < read_parameter(c.first + 1, lv).i;
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (pass)
			{
				for (i = i + 1; i <= end_i; )
					do_cmd();
			}
			i = end_i + 1;
		}
			break;
		case tRestoreHP:
			if (c.second >= 2)
				character->restore_hp(read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case tRestoreMP:
			if (c.second >= 2)
				character->restore_mp(read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case tTakeDamage:
			if (c.second >= 2)
				character->take_damage(MagicDamage, read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case tTakeDamagePct:
			if (c.second >= 2)
				character->take_damage(MagicDamage, character->hp_max * (read_parameter(c.first + 1, lv).i / 100.f));
			i++;
			break;
		case tInflictDamge:
			if (c.second >= 3)
				character->inflict_damage(target_character, (DamageType)read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).i);
			i++;
			break;
		case tLevelUp:
			character->gain_exp(character->exp_max);
			i++;
			break;
		case tIncreaseHPMax:
			if (c.second >= 2)
				character->hp_max += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseMPMax:
			if (c.second >= 2)
				character->mp_max += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseATK:
			if (c.second >= 2)
				character->atk += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreasePHYDEF:
			if (c.second >= 2)
				character->phy_def += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseMAGDEF:
			if (c.second >= 2)
				character->mag_def += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseHPREG:
			if (c.second >= 2)
				character->hp_reg += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseMPREG:
			if (c.second >= 2)
				character->mp_reg += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseMOVSP:
			if (c.second >= 2)
				character->mov_sp += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseATKSP:
			if (c.second >= 2)
				character->atk_sp += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case tIncreaseHPMaxPct:
			if (c.second >= 2)
				character->hp_max *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case tIncreaseMPMaxPct:
			if (c.second >= 2)
				character->mp_max *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case tIncreaseATKPct:
			if (c.second >= 2)
				character->atk *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case tAddBuff:
			if (c.second >= 3)
				character->add_buff(read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).f, c.second >= 3 ? read_parameter(c.first + 3, lv).u : 0, c.second >= 4 ? read_parameter(c.first + 4, lv).u : false);
			i++;
			break;
		case tAddBuffToTarget:
			if (c.second >= 3)
				target_character->add_buff(read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).f, c.second >= 3 ? read_parameter(c.first + 3, lv).u : 0, c.second >= 4 ? read_parameter(c.first + 4, lv).u : false);
			i++;
			break;
		case tAddAttackEffect:
		{
			auto& ef = character->attack_effects.emplace_back();

			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			auto in_sub = end_i != i + 1;
			for (i = i + 1 + (in_sub ? 1 : 0); 
				i <= end_i - (in_sub ? 1 : 0); i++)
			{
				auto& c = cmds[i];

				ef.cmds.emplace_back((int)ef.data.size(), c.second);
				for (auto j = 0; j < c.second; j++)
				{
					ef.data.push_back(data[c.first + j]);
					ef.use_ext_para.push_back(0);
				}
				ef.init_sub_groups();
			}

			i = end_i + 1;
		}
			break;
		case tTeleportToTarget:
			teleport(character, target_pos);
			i++;
			break;
		}
	};

	for (; i < cmds.size(); )
		do_cmd();
}

void CommandList::build(const std::vector<std::string>& tokens)
{
	static auto ei_type = find_enum(th<Type>());

	for (auto& t : tokens)
	{
		auto sp = SUS::split(t, ',');
		cmds.emplace_back((int)data.size(), (int)sp.size());
		for (auto& tt : sp)
		{
			if (std::isdigit(tt[0]))
			{
				data.push_back(read_variant(tt));
				use_ext_para.push_back(0);
			}
			else
			{
				if (auto ii = ei_type->find_item(tt); ii)
				{
					data.push_back({ .i = ii->value });
					use_ext_para.push_back(0);
				}
				else if (int id; parse_literal(tt, id))
				{
					data.push_back({ .i = id });
					use_ext_para.push_back(0);
				}
				else
				{
					data.push_back({ .u = sh(tt.c_str()) });
					use_ext_para.push_back(1);
				}
			}
		}
	}

	init_sub_groups();
}
