#include "command.h"
#include "character.h"
#include "effect.h"
#include "collider.h"

#define FLAME_NO_XML
#define FLAME_NO_JSON
#include <flame/foundation/typeinfo_serialize.h>
#include <flame/universe/components/node.h>
#include <flame/universe/components/nav_agent.h>

Parameter::Parameter(const std::string& str)
{
	if (str.empty() || str == "_")
	{
		type = tEmpty;
		return;
	}

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
			type = tVariable;
			CommandList::Variable sv;
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
	static lVariant zero_reg = { .p = nullptr };
	lVariant reg[8] = { {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr}, {.p = nullptr} };
	int l; uint ul;
	int i = 0; int loop_n; int loop_end_i;
	auto character_pos = character->node->pos;

	auto variable_addr = [&](int sv, voidptr& ptr, uint& size) {
		switch (sv)
		{
		case vCharacter:
			ptr = &character;
			size = (uint)sizeof(void*);
			break;
		case vTargetCharacter:
			ptr = &target_character;
			size = (uint)sizeof(void*);
			break;
		case vREG0:
			ptr = &reg[0];
			size = (uint)sizeof(lVariant);
			break;
		case vREG1:
			ptr = &reg[1];
			size = (uint)sizeof(lVariant);
			break;
		case vREG2:
			ptr = &reg[2];
			size = (uint)sizeof(lVariant);
			break;
		case vREG3:
			ptr = &reg[3];
			size = (uint)sizeof(lVariant);
			break;
		case vREG4:
			ptr = &reg[4];
			size = (uint)sizeof(lVariant);
			break;
		case vREG5:
			ptr = &reg[5];
			size = (uint)sizeof(lVariant);
			break;
		case vREG6:
			ptr = &reg[6];
			size = (uint)sizeof(lVariant);
			break;
		case vREG7:
			ptr = &reg[7];
			size = (uint)sizeof(lVariant);
			break;
		case vZeroREG:
			ptr = &zero_reg;
			size = (uint)sizeof(lVariant);
			break;
		}
	};

	auto variable_as = [&]<typename T>(int sv)->T& {
		void* ptr; uint sz;
		variable_addr(sv, ptr, sz);
		return *(T*)ptr;
	};

	auto parameter_addr = [&](Parameter& parameter, voidptr& ptr, uint& size) {
		if (parameter.type == Parameter::tVariable)
			variable_addr(parameter.u.v.i, ptr, size);
		else
		{
			ptr = &parameter.u.v;
			size = sizeof(Parameter::u.v);
		}
	};

	auto add_callback = [&](int i, int end_i, CommandList& cl) {
		auto in_sub = end_i != i + 1;
		for (auto j = i + 1; j <= end_i; j++)
		{
			if (in_sub)
			{
				if (j == i + 1 || j == end_i)
					continue;
			}
			cl.cmds.push_back(cmds[j]);
		}
		cl.init_sub_groups();
		for (auto& c : cl.cmds)
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
	};

	auto set_effect_special_init_data = [&](cEffectPtr effect, const Parameter& parameter) {
		void* ptr = nullptr;
		variable_addr(parameter.to_i(), ptr, ul);
		if (effect->special_effect)
			effect->special_effect->init(ptr, ul);
	};

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
				parameters.push_back(vec.size() == 1 ? vec[0] : vec[lv - 1]);
			}
				break;
			case Parameter::tVariable:
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
						ret = vec.size() == 1 ? vec[0] : vec[lv - 1];
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
				parameter_addr(parameters[0], src_ptr, ul); l = ul;
				parameter_addr(parameters[1], dst_ptr, ul);
				memcpy(dst_ptr, src_ptr, min((uint)l, ul));
			}
			i++;
			break;
		case cBitInverse:
			if (parameters.size() >= 1)
			{
				auto& v = variable_as.operator()<uint>(parameters[0].to_i());
				v = ~v;
			}
			i++;
			break;
		case cIfEqual:
		{
			auto beg_i = i;
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (parameters.size() >= 2)
			{
				void* src_ptr = nullptr;
				void* dst_ptr = nullptr;
				parameter_addr(parameters[0], src_ptr, ul); l = ul;
				parameter_addr(parameters[1], dst_ptr, ul);
				if (memcmp(src_ptr, dst_ptr, min((uint)l, ul)) == 0)
				{
					i = i + 1 + (end_i != i + 1 ? 1 : 0);
					for (; i <= end_i; )
						do_cmd();
				}
			}

			if (i >= beg_i && i <= end_i)
				i = end_i + 1;
		}
			break;
		case cIfNotEqual:
		{
			auto beg_i = i;
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (parameters.size() >= 2)
			{
				void* src_ptr = nullptr;
				void* dst_ptr = nullptr;
				parameter_addr(parameters[0], src_ptr, ul); l = ul;
				parameter_addr(parameters[1], dst_ptr, ul);
				if (memcmp(src_ptr, dst_ptr, min((uint)l, ul)) != 0)
				{
					i = i + 1 + (end_i != i + 1 ? 1 : 0);
					for (; i <= end_i; )
						do_cmd();
				}
			}

			if (i >= beg_i && i <= end_i)
				i = end_i + 1;
		}
			break;
		case cLoop:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;
			loop_end_i = end_i + 1;

			if (parameters.size() >= 1)
			{
				auto beg_i = i + 1 + (end_i != i + 1 ? 1 : 0);
				loop_n = parameters[0].to_i();
				for (auto j = 0; j < loop_n; j++)
				{
					for (i = beg_i; i <= end_i; )
						do_cmd();
				}
			}

			i = end_i + 1;
		}
			break;
		case cBreak:
			loop_n = 0;
			i = loop_end_i;
			break;
		case cGenerateRnd:
			if (parameters.size() >= 1)
				variable_as.operator()<uint>(parameters[0].to_i()) = rand();
			i++;
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
		case cGetFaction:
			if (parameters.size() >= 2)
				variable_as.operator()<uint>(parameters[1].to_i()) = variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->faction;
			i++;
			break;
		case cForNearbyCharacters:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (parameters.size() >= 2)
			{
				auto faction = parameters[0].to_i();
				auto search_range = parameters[1].to_f();
				auto start_radius = parameters.size() >= 3 ? parameters[2].to_f() : 0.f;
				auto central_angle = parameters.size() >= 4 ? parameters[3].to_f() : 0.f;
				auto direction_angle = central_angle > 0.f ? angle_xz(character_pos, target_pos) : 0.f;
				auto beg_i = i + 1;
				for (auto c : find_characters(faction, character_pos, search_range, start_radius, central_angle, direction_angle))
				{
					reg[0].p = c;
					for (i = beg_i; i <= end_i; )
						do_cmd();
				}
			}

			i = end_i + 1;
		}
			break;
		case cNearestCharacter:
			if (parameters.size() >= 3)
			{
				auto character = variable_as.operator()<cCharacterPtr>(parameters[0].to_i());
				auto faction = variable_as.operator()<uint>(parameters[1].to_i());
				auto res = find_characters(faction, character->node->pos, parameters[2].to_f());
				reg[0].p = nullptr;
				auto marker = 0U;
				auto marker_time = 0.f;
				if (parameters.size() >= 5)
				{
					marker = variable_as.operator() < uint > (parameters[3].to_i());
					marker_time = parameters[4].to_f();
				}
				for (auto c : res)
				{
					if (c != character)
					{
						if (marker && c->add_marker(marker, marker_time))
						{
							reg[0].p = c;
							break;
						}
					}
				}
			}
			i++;
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
			{
				if (parameters[1].vt == Parameter::vPercentage)
					character->take_damage((DamageType)parameters[0].to_i(), character->hp_max * parameters[1].to_f());
				else
					character->take_damage((DamageType)parameters[0].to_i(), parameters[1].to_i());
			}
			i++;
			break;
		case cInflictDamage:
			if (parameters.size() >= 4)
				variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->inflict_damage(variable_as.operator()<cCharacterPtr>(parameters[1].to_i()), (DamageType)parameters[2].to_i(), parameters[3].to_i());
			i++;
			break;
		case cLevelUp:
			character->gain_exp(character->exp_max);
			i++;
			break;
		case cIncreaseHPMax:
			if (parameters.size() >= 1)
			{
				if (parameters[0].vt == Parameter::vPercentage)
					character->hp_max *= 1.f + parameters[0].to_f();
				else
					character->hp_max += parameters[0].to_i();
			}
			i++;
			break;
		case cIncreaseMPMax:
			if (parameters.size() >= 1)
			{
				if (parameters[0].vt == Parameter::vPercentage)
					character->mp_max *= 1.f + parameters[0].to_f();
				else
					character->mp_max += parameters[0].to_i();
			}
			i++;
			break;
		case cIncreaseATK:
			if (parameters.size() >= 1)
			{
				if (parameters[0].vt == Parameter::vPercentage)
					character->atk *= 1.f + parameters[0].to_f();
				else
					character->atk += parameters[0].to_i();
			}
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
		case cAddBuff:
			if (parameters.size() >= 3)
				variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->add_buff(parameters[1].to_i(), parameters[2].to_f(), parameters.size() >= 4 ? parameters[3].to_i() : 0, parameters.size() >= 5 ? parameters[4].to_i() : false);
			i++;
			break;
		case cAddAttackEffect:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			add_callback(i, end_i, character->attack_effects.emplace_back());

			i = end_i + 1;
		}
			break;
		case cSetSectorCollideCallback:
		{
			auto end_i = i + 1;
			if (auto it = sub_groups.find(i + 1); it != sub_groups.end())
				end_i = it->second;

			if (parameters.size() >= 1)
			{
				if (auto entity = variable_as.operator()<EntityPtr>(parameters[0].to_i()); entity)
				{
					if (auto collider = entity->get_component_t<cSectorCollider>(); collider)
					{
						collider->faction = ~character->faction;
						collider->host = character;
						add_callback(i, end_i, collider->callback);
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
			{
				auto effect = add_effect(parameters[0].to_i(), character_pos, vec3(0.f), parameters[1].to_f());
				if (parameters.size() >= 3)
					set_effect_special_init_data(effect, parameters[2]);
				reg[0].p = effect->entity;
			}
			i++;
			break;
		case cAddEffectToCharacter:
			if (parameters.size() >= 3)
			{
				auto effect = add_effect(parameters[1].to_i(), vec3(0.f), vec3(0.f), parameters[2].to_f(), variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->entity);
				if (parameters.size() >= 4)
					set_effect_special_init_data(effect, parameters[3]);
				reg[0].p = effect->entity;
			}
			i++;
			break;
		case cAddEffectFaceTarget:
			if (parameters.size() >= 2)
			{
				auto effect = add_effect(parameters[0].to_i(), character_pos + vec3(0.f, character->nav_agent->height * 0.5f, 0.f), vec3(angle_xz(character_pos, target_pos), 0.f, 0.f), parameters[1].to_f());
				if (parameters.size() >= 3)
					set_effect_special_init_data(effect, parameters[2]);
				reg[0].p = effect->entity;
			}
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

bool CommandListExecuteThread::step()
{

	return true;
}
