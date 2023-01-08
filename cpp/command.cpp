#include "command.h"
#include "character.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

Parameter::Parameter(const std::string& str)
{
	if (std::isdigit(str[0]))
	{
		switch (str.back())
		{
		case 'f':
			vt = vFloat;
			u.v.f = s2t<float>(str);
			break;
		case 'u':
			vt = vUint;
			u.v.u = s2t<uint>(str);
			break;
		case '%':
			vt = vPercentage;
			u.v.i = s2t<uint>(str);
			break;
		case 'i':
		default:
			vt = vInt;
			u.v.i = s2t<int>(str);
		}
	}
	else
	{
		if (str[0] == '%')
		{
			type = tSpecialVariable;
			SpecialVariable sv;
			TypeInfo::unserialize_t(str.substr(1), sv);
			u.v.i = sv;
		}
		else if (int id; parse_literal(str, id))
		{
			type = tImmediate;
			u.v.i = id;
		}
		else
		{
			type = tExternal;
			vt = vUint;
			u.v.u = sh(str.c_str());
		}
	}
}

void read_parameters(ParameterNames& parameter_names, ParameterPack& parameters, const std::vector<std::string>& tokens)
{
	for (auto& t : tokens)
	{
		auto sp = SUS::split(t, ':');
		auto hash = sh(sp[0].c_str());
		parameter_names.emplace_back(sp[0], hash);
		auto& vec = parameters.emplace(hash, std::vector<Parameter>()).first->second;
		for (auto& tt : SUS::split(sp[1], '/'))
			vec.push_back(Parameter(tt));
	}
}

void CommandList::init_sub_groups()
{
	std::stack<uint> sg_stack;
	for (auto i = 0; i < cmds.size(); i++)
	{
		auto c = cmds[i].first;
		if (c == cBeginSub)
			sg_stack.push(i);
		else if (c == cEndSub)
		{
			auto idx = sg_stack.top();
			sg_stack.pop();
			sub_groups.emplace(idx, i);
		}
	}
}

void CommandList::execute(cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) const
{
	lVariant reg[4] = { {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr} };
	auto character_pos = character->node->pos;
	auto special_variable_info = [&](Parameter::SpecialVariable sv, voidptr& ptr, uint& size) {
		switch (sv)
		{
		case Parameter::sREG0:
			ptr = &reg[0];
			size = size > 0 ? min(size, (uint)sizeof(lVariant)) : (uint)sizeof(lVariant);
			break;
		case Parameter::sREG1:
			ptr = &reg[1];
			size = size > 0 ? min(size, (uint)sizeof(lVariant)) : (uint)sizeof(lVariant);
			break;
		case Parameter::sREG2:
			ptr = &reg[2];
			size = size > 0 ? min(size, (uint)sizeof(lVariant)) : (uint)sizeof(lVariant);
			break;
		case Parameter::sREG3:
			ptr = &reg[3];
			size = size > 0 ? min(size, (uint)sizeof(lVariant)) : (uint)sizeof(lVariant);
			break;
		case Parameter::sTargetCharacter:
			ptr = &target_character;
			size = size > 0 ? min(size, (uint)sizeof(void*)) : (uint)sizeof(void*);
			break;
		}
	};

	auto i = 0;
	std::function<void()> do_cmd;
	do_cmd = [&]() {
		auto& cmd = cmds[i];

		std::vector<Parameter> parameters;
		for (auto i_parm = 0; i_parm < cmd.second.size(); i_parm++)
		{
			auto& sp = cmd.second[i_parm];
			switch (sp.type)        
			{
			case Parameter::tImmediate:
				parameters.push_back(sp);
				break;
			case Parameter::tExternal:
			{
				auto& vec = external_parameters.at(sp.u.v.u);
				parameters.push_back(lv <= vec.size() ? vec[lv - 1] : vec[0]);
			}
				break;
			case Parameter::tSpecialVariable:
				parameters.push_back(sp);
				break;
			case Parameter::tExpression:
			{
				auto get_operand = [&]()->Parameter {
					auto ret = cmd.second[i_parm++];
					assert(ret.type != Parameter::tExpression);
					if (ret.type == Parameter::tExternal)
					{
						ret.type = Parameter::tImmediate;
						auto& vec = external_parameters.at(ret.u.v.u);
						ret = lv <= vec.size() ? vec[lv - 1] : vec[0];
					}
					return ret;
				};
				i_parm++;
				switch (sp.u.e.op)
				{
				case Parameter::OpAdd:
				{
					auto num1 = get_operand();
					auto num2 = get_operand();
					parameters.emplace_back(num1.to_f() + num2.to_f());
				}
					break;
				case Parameter::OpMinus:
				{
					auto num1 = get_operand();
					auto num2 = get_operand();
					parameters.emplace_back(num1.to_f() - num2.to_f());
				}
					break;
				case Parameter::OpMultiply:
				{
					auto num1 = get_operand();
					auto num2 = get_operand();
					parameters.emplace_back(num1.to_f() * num2.to_f());
				}
					break;
				case Parameter::OpDivide:
				{
					auto num1 = get_operand(); 
					auto num2 = get_operand();
					parameters.emplace_back(num1.to_f() / num2.to_f());
				}
					break;
				}
			}
				break;
			}
		}

		switch (cmd.first)
		{
		case cStore:
			if (parameters.size() >= 2)
			{
				void* src_ptr = nullptr;
				void* dst_ptr = nullptr;
				auto size = 0U;
				special_variable_info((Parameter::SpecialVariable)parameters[0].to_i(), src_ptr, size);
				special_variable_info((Parameter::SpecialVariable)parameters[1].to_i(), dst_ptr, size);
				memcpy(dst_ptr, src_ptr, size);
			}

			i++;
			break;
		case cIfEqual:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			void* src_ptr = nullptr;
			void* dst_ptr = nullptr;
			auto size = 0U;
			special_variable_info((Parameter::SpecialVariable)parameters[0].to_i(), src_ptr, size);
			special_variable_info((Parameter::SpecialVariable)parameters[1].to_i(), dst_ptr, size);
			if (memcmp(src_ptr, dst_ptr, size) == 0)
			{
				for (i = i + 1; i <= end_i; )
					do_cmd();
			}

			i = end_i + 1;
		}
			break;
		case cIfNotEqual:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			void* src_ptr = nullptr;
			void* dst_ptr = nullptr;
			auto size = 0U;
			special_variable_info((Parameter::SpecialVariable)parameters[0].to_i(), src_ptr, size);
			special_variable_info((Parameter::SpecialVariable)parameters[1].to_i(), dst_ptr, size);
			if (memcmp(src_ptr, dst_ptr, size) != 0)
			{
				for (i = i + 1; i <= end_i; )
					do_cmd();
			}

			i = end_i + 1;
		}
			break;
		case cRollDice100:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (parameters.size() >= 1 && linearRand(0U, 99U) < parameters[0].to_i())
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

			if (parameters.size() >= 1)
			{
				auto search_range = parameters[0].to_f();
				auto search_angle = parameters.size() >= 2 ? parameters[1].to_f() : 0.f;
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
			if (parameters.size() >= 1)
				character->restore_hp(parameters[0].to_i());
			i++;
			break;
		case cRestoreMP:
			if (parameters.size() >= 1)
				character->restore_mp(parameters[0].to_i());
			i++;
			break;
		case cTakeDamage:
			if (parameters.size() >= 2)
				character->take_damage((DamageType)parameters[0].to_i(), parameters[1].to_i());
			i++;
			break;
		case cTakeDamagePct:
			if (parameters.size() >= 2)
				character->take_damage((DamageType)parameters[0].to_i(), character->hp_max * parameters[1].to_f());
			i++;
			break;
		case cInflictDamage:
			if (parameters.size() >= 2)
				character->inflict_damage(target_character, (DamageType)parameters[0].to_i(), parameters[1].to_i());
			i++;
			break;
		case cLevelUp:
			character->gain_exp(character->exp_max);
			i++;
			break;
		case cIncreaseHPMax:
			if (parameters.size() >= 1)
				character->hp_max += parameters[0].to_i();
			i++;
			break;
		case cIncreaseMPMax:
			if (parameters.size() >= 1)
				character->mp_max += parameters[0].to_i();
			i++;
			break;
		case cIncreaseATK:
			if (parameters.size() >= 1)
				character->atk += parameters[0].to_i();
			i++;
			break;
		case cIncreasePHYDEF:
			if (parameters.size() >= 1)
				character->phy_def += parameters[0].to_i();
			i++;
			break;
		case cIncreaseMAGDEF:
			if (parameters.size() >= 1)
				character->mag_def += parameters[0].to_i();
			i++;
			break;
		case cIncreaseHPREG:
			if (parameters.size() >= 1)
				character->hp_reg += parameters[0].to_i();
			i++;
			break;
		case cIncreaseMPREG:
			if (parameters.size() >= 1)
				character->mp_reg += parameters[0].to_i();
			i++;
			break;
		case cIncreaseMOVSP:
			if (parameters.size() >= 1)
				character->mov_sp += parameters[0].to_i();
			i++;
			break;
		case cIncreaseATKSP:
			if (parameters.size() >= 1)
				character->atk_sp += parameters[0].to_i();
			i++;
			break;
		case cIncreaseHPMaxPct:
			if (parameters.size() >= 1)
				character->hp_max *= 1.f + parameters[0].to_f();
			i++;
			break;
		case cIncreaseMPMaxPct:
			if (parameters.size() >= 1)
				character->mp_max *= 1.f + parameters[0].to_f();
			i++;
			break;
		case cIncreaseATKPct:
			if (parameters.size() >= 1)
				character->atk *= 1.f + parameters[0].to_f();
			i++;
			break;
		case cAddBuff:
			if (parameters.size() >= 2)
				character->add_buff(parameters[0].to_i(), parameters[1].to_f(), parameters.size() >= 3 ? parameters[2].to_i() : 0, parameters.size() >= 4 ? parameters[3].to_i() : false);
			i++;
			break;
		case cAddBuffToTarget:
			if (parameters.size() >= 2)
				target_character->add_buff(parameters[0].to_i(), parameters[1].to_f(), parameters.size() >= 3 ? parameters[2].to_i() : 0, parameters.size() >= 4 ? parameters[3].to_i() : false);
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
				ef.cmds.push_back(cmds[i]);
			ef.init_sub_groups();
			for (auto& c : ef.cmds)
			{
				for (auto& p : c.second)
				{
					if (p.type == Parameter::tExternal)
					{
						if (auto it = external_parameters.find(p.u.v.u); it != external_parameters.end())
							p = it->second[lv - 1];
					}
				}
			}

			i = end_i + 1;
		}
			break;
		case cTeleportToTarget:
			teleport(character, target_character ? target_character->node->pos : target_pos);
			i++;
			break;
		case cAddEffect:
			if (parameters.size() >= 2)
				add_effect(parameters[0].to_i(), character_pos, vec3(0.f), parameters[1].to_f());
			i++;
			break;
		case cAddEffectFaceTarget:
			if (parameters.size() >= 2)
				add_effect(parameters[0].to_i(), character_pos + vec3(0.f, character->nav_agent->height * 0.5f, 0.f), vec3(angle_xz(character_pos, target_pos), 0.f, 0.f), parameters[1].to_f());
			i++;
			break;
		default:
			i++;
		}
	};

	for (; i < cmds.size(); )
		do_cmd();
}

void CommandList::build(const std::vector<std::string>& tokens)
{
	static auto reg_exp = std::regex(R"(([\w]+)([\+\-\*\/])([\w]+))");
	std::smatch res;

	for (auto& t : tokens)
	{
		auto sp = SUS::split(t, ',');
		auto& c = cmds.emplace_back();
		TypeInfo::unserialize_t(sp[0], c.first);

		for (auto i = 1; i < sp.size(); i++)
		{
			auto& tt = sp[i];
			if (std::regex_search(tt, res, reg_exp))
			{
				auto& p = c.second.emplace_back();
				p.type = Parameter::tExpression;
				switch (res[2].str()[0])
				{
				case '+': p.u.e.op = Parameter::OpAdd; break;
				case '-': p.u.e.op = Parameter::OpMinus; break;
				case '*': p.u.e.op = Parameter::OpMultiply; break;
				case '/': p.u.e.op = Parameter::OpDivide; break;
				}

				c.second.push_back(Parameter(res[1].str()));
				c.second.push_back(Parameter(res[3].str()));
			}
			else
				c.second.push_back(Parameter(tt));
		}
	}

	init_sub_groups();
}
