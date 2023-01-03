#include "command.h"
#include "character.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/universe/components/node.h>

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
		if (type == cBeginSub)
			sg_stack.push(i);
		else if (type == cEndSub)
		{
			auto idx = sg_stack.top();
			sg_stack.pop();
			sub_groups.emplace(idx, i);
		}
	}
}

void CommandList::execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const
{
	auto read_parameter = [&](uint& idx, uint lv)->Variant {
		auto it = non_immediates.find(idx);
		if (it == non_immediates.end())
			return data[idx];
		switch (it->second)
		{
		case 1:
		{
			auto& vec = external_parameters.at(data[idx].u);
			return lv <= vec.size() ? vec[lv] : vec[0];
		}
			break;
		case 2:
		{
			union
			{
				Variant v;
				Expression e;
			}cvt;
			cvt.v = data[idx];
		}
			break;
		}
		return { .i=0 };
	};

	auto character_pos = character->node->pos;

	auto i = 0;
	std::function<void()> do_cmd;
	do_cmd = [&]() {
		auto& c = cmds[i];
		auto parm_idx = c.first + 1;
		switch (data[c.first].i)
		{
		case cIfEqual:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			i = end_i + 1;
		}
			break;
		case cRollDice100:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (c.second >= 2 && linearRand(0U, 99U) < read_parameter(c.first + 1, lv).i)
			{
				for (i = i + 1; i <= end_i; )
					do_cmd();
			}
			i = end_i + 1;
		}
			break;
		case cForNearbyEnemies:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (c.second >= 2)
			{
				auto search_range = read_parameter(c.first + 1, lv).f;
				auto search_angle = c.second >= 3 ? read_parameter(c.first + 2, lv).f : 0.f;
				auto target_angle = search_angle > 0.f ? angle_xz(character_pos, target_pos) : 0.f;
				auto ori_target_character = target_character;
				auto beg_i = i + 1;
				for (auto c : find_characters(character_pos, search_range, ~character->faction))
				{
					if (search_angle > 0.f)
					{
						if (abs(angle_diff(target_angle, angle_xz(character_pos, c->node->pos))) > search_angle)
							continue;
					}

					target_character = c;
					for (i = beg_i; i <= end_i; )
						do_cmd();
				}
				target_character = ori_target_character;
			}

			i = end_i + 1;
		}
			break;
		case cRestoreHP:
			if (c.second >= 2)
				character->restore_hp(read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case cRestoreMP:
			if (c.second >= 2)
				character->restore_mp(read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case cTakeDamage:
			if (c.second >= 2)
				character->take_damage(MagicDamage, read_parameter(c.first + 1, lv).i);
			i++;
			break;
		case cTakeDamagePct:
			if (c.second >= 2)
				character->take_damage(MagicDamage, character->hp_max * (read_parameter(c.first + 1, lv).i / 100.f));
			i++;
			break;
		case cInflictDamge:
			if (c.second >= 3)
				character->inflict_damage(target_character, (DamageType)read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).i);
			i++;
			break;
		case cLevelUp:
			character->gain_exp(character->exp_max);
			i++;
			break;
		case cIncreaseHPMax:
			if (c.second >= 2)
				character->hp_max += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseMPMax:
			if (c.second >= 2)
				character->mp_max += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseATK:
			if (c.second >= 2)
				character->atk += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreasePHYDEF:
			if (c.second >= 2)
				character->phy_def += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseMAGDEF:
			if (c.second >= 2)
				character->mag_def += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseHPREG:
			if (c.second >= 2)
				character->hp_reg += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseMPREG:
			if (c.second >= 2)
				character->mp_reg += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseMOVSP:
			if (c.second >= 2)
				character->mov_sp += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseATKSP:
			if (c.second >= 2)
				character->atk_sp += read_parameter(c.first + 1, lv).i;
			i++;
			break;
		case cIncreaseHPMaxPct:
			if (c.second >= 2)
				character->hp_max *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case cIncreaseMPMaxPct:
			if (c.second >= 2)
				character->mp_max *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case cIncreaseATKPct:
			if (c.second >= 2)
				character->atk *= (100 + read_parameter(c.first + 1, lv).i) / 100.f;
			i++;
			break;
		case cAddBuff:
			if (c.second >= 3)
				character->add_buff(read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).f, c.second >= 3 ? read_parameter(c.first + 3, lv).u : 0, c.second >= 4 ? read_parameter(c.first + 4, lv).u : false);
			i++;
			break;
		case cAddBuffToTarget:
			if (c.second >= 3)
				target_character->add_buff(read_parameter(c.first + 1, lv).i, read_parameter(c.first + 2, lv).f, c.second >= 3 ? read_parameter(c.first + 3, lv).u : 0, c.second >= 4 ? read_parameter(c.first + 4, lv).u : false);
			i++;
			break;
		case cAddAttackEffect:
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
					auto idx = c.first + j;
					ef.data.push_back(data[idx]);
					if (auto it = non_immediates.find(idx); it != non_immediates.end())
						ef.non_immediates[(uint)ef.data.size() - 1] = it->second;
				}
				ef.init_sub_groups();
			}

			i = end_i + 1;
		}
			break;
		case cTeleportToTarget:
			teleport(character, target_character ? target_character->node->pos : target_pos);
			i++;
			break;
		case cAddEffect:
			if (c.second >= 3)
				add_effect(read_parameter(c.first + 1, lv).u, character_pos, vec3(0.f), read_parameter(c.first + 2, lv).f);
			i++;
			break;
		case cAddEffectFaceTarget:
			if (c.second >= 3)
				add_effect(read_parameter(c.first + 1, lv).u, character_pos, vec3(angle_xz(character_pos, target_pos), 0.f, 0.f), read_parameter(c.first + 2, lv).f);
			i++;
			break;
		}
	};

	for (; i < cmds.size(); )
		do_cmd();
}

void CommandList::build(const std::vector<std::string>& tokens)
{
	static auto ei_type = find_enum(th<Command>());

	for (auto& t : tokens)
	{
		auto sp = SUS::split(t, ',');
		cmds.emplace_back((int)data.size(), (int)sp.size());
		for (auto& tt : sp)
		{
			static auto reg_exp = std::regex(R"(([\w]+)([\+\-\*\/])([\w]+))");
			std::smatch res;

			if (std::isdigit(tt[0]))
				data.push_back(read_variant(tt));
			else if (std::regex_search(tt, res, reg_exp))
			{
				auto op = OpNull;
				switch (res[2].str()[0])
				{
				case '+': op = OpAdd; break;
				case '-': op = OpMinus; break;
				case '*': op = OpMultiply; break;
				case '/': op = OpDivide; break;
				}

				auto data_off = (uint)data.size();

				union
				{
					Variant v;
					Expression e;
				}cvt;
				cvt.e = { op, data_off + 1, data_off + 2 };
				data.push_back(cvt.v);
				non_immediates[(uint)data.size() - 1] = 2;

				data.push_back({ .u = sh(res[1].str().c_str())});
				non_immediates[(uint)data.size() - 1] = 1;

				data.push_back({ .u = sh(res[3].str().c_str()) });
				non_immediates[(uint)data.size() - 1] = 1;
			}
			else
			{
				if (auto ii = ei_type->find_item(tt); ii)
					data.push_back({ .i = ii->value });
				else if (int id; parse_literal(tt, id))
					data.push_back({ .i = id });
				else
				{
					data.push_back({ .u = sh(tt.c_str()) });
					non_immediates[(uint)data.size() - 1] = 1;
				}
			}
		}
	}

	init_sub_groups();
}
