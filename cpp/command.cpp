#include "command.h"
#include "object.h"
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
			if (str[1] == '_' || std::islower(str[1]))
				u.v.u = sh(str.c_str());
			else
			{
				CommandList::Variable sv;
				TypeInfo::unserialize_t(str.substr(1), sv);
				u.v.i = sv;
			}
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
		auto c = std::get<0>(cmds[i]);
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

void CommandList::build(const std::vector<std::string>& tokens)
{
	static auto reg_ret = std::regex(R"((%[\w]+)\:\=(.*))");
	static auto reg_exp = std::regex(R"(([\w]+)([\+\-\*\/])([\w]+))");
	std::smatch res;

	uint used_reg[vREG7 - vREG0 + 1];
	memset(used_reg, 0, sizeof(used_reg));
	auto mark_used_reg = [&](uint v) {
		if (v >= vREG0 && v <= vREG7)
			used_reg[v - vREG0] = 1;
	};
	auto find_reg = [&](uint h)->Variable {
		for (auto i = 0; i < countof(used_reg); i++)
		{
			if (used_reg[i] == h)
				return (Variable)(vREG0 + i);
		}
		for (auto i = 0; i < countof(used_reg); i++)
		{
			if (used_reg[i] == 0)
			{
				used_reg[i] = h;
				return (Variable)(vREG0 + i);
			}
		}
	};

	for (auto& t : tokens)
	{
		if (t.empty()) continue;

		auto& cmd = cmds.emplace_back();
		std::get<2>(cmd) = vNull;

		std::vector<std::string> sp;
		if (std::regex_search(t, res, reg_ret))
		{
			std::get<2>(cmd) = (Variable)Parameter(res[1].str()).u.v.u;
			sp = SUS::split(res[2], ',');
		}
		else
			sp = SUS::split(t, ',');

		TypeInfo::unserialize_t(sp[0], std::get<0>(cmd));

		for (auto i = 1; i < sp.size(); i++)
		{
			auto& tt = sp[i];
			if (std::regex_search(tt, res, reg_exp))
			{
				auto& p = std::get<1>(cmd).emplace_back();
				p.type = Parameter::tExpression;
				switch (res[2].str()[0])
				{
				case '+': p.u.e.op = Parameter::OpAdd; break;
				case '-': p.u.e.op = Parameter::OpMinus; break;
				case '*': p.u.e.op = Parameter::OpMultiply; break;
				case '/': p.u.e.op = Parameter::OpDivide; break;
				}

				std::get<1>(cmd).push_back(Parameter(res[1].str()));
				std::get<1>(cmd).push_back(Parameter(res[3].str()));
			}
			else
				std::get<1>(cmd).push_back(Parameter(tt));
		}
	}

	for (auto& cmd : cmds)
	{
		if (std::get<2>(cmd) != vNull)
			mark_used_reg(std::get<2>(cmd));
		for (auto& p : std::get<1>(cmd))
		{
			if (p.type == Parameter::tVariable)
				mark_used_reg(p.u.v.i);
		}
	}
	for (auto& cmd : cmds)
	{
		if (std::get<2>(cmd) != vNull && std::get<2>(cmd) > vCount)
			std::get<2>(cmd) = find_reg(std::get<2>(cmd));
		for (auto& p : std::get<1>(cmd))
		{
			if (p.type == Parameter::tVariable && p.u.v.u > vCount)
				p.u.v.i = find_reg(p.u.v.i);
		}
	}

	init_sub_groups();
}

CommandListExecuteThread::CommandListExecuteThread(const CommandList& cl, cCharacterPtr character, cCharacterPtr target_character, const vec3& target_pos, const ParameterPack& external_parameters, uint lv) :
	cl(cl),
	character(character),
	target_character(target_character),
	target_pos(target_pos),
	external_parameters(external_parameters),
	lv(lv)
{
	auto& frame = frames.emplace();
	frame.beg_i = 0;
	frame.end_i = cl.cmds.size() - 1;
	frame.i = 0;
}

void CommandListExecuteThread::execute()
{
	static lVariant zero_reg = { .p = nullptr };
	uint ul, ul2;

	auto variable_addr = [&](int sv, voidptr& ptr, uint& size) {
		switch (sv)
		{
		case CommandList::vCharacter:
			ptr = &character;
			size = (uint)sizeof(void*);
			break;
		case CommandList::vTargetCharacter:
			ptr = &target_character;
			size = (uint)sizeof(void*);
			break;
		case CommandList::vREG0:
			ptr = &reg[0];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG1:
			ptr = &reg[1];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG2:
			ptr = &reg[2];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG3:
			ptr = &reg[3];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG4:
			ptr = &reg[4];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG5:
			ptr = &reg[5];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG6:
			ptr = &reg[6];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vREG7:
			ptr = &reg[7];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vVEC0:
			ptr = &vec[0];
			size = (uint)sizeof(lVariant);
			break;
		case CommandList::vZeroREG:
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

	auto add_callback = [&](int beg_i, int end_i, CommandList& new_cl) {
		for (auto j = beg_i; j <= end_i; j++)
			new_cl.cmds.push_back(cl.cmds[j]);
		new_cl.init_sub_groups();
		for (auto& c : new_cl.cmds)
		{
			for (auto& p : std::get<1>(c))
			{
				if (p.type == Parameter::tExternal)
				{
					if (auto it = external_parameters.find(p.u.v.u); it != external_parameters.end())
						p = it->second[lv - 1];
				}
			}
		}
	};

	auto& frame = frames.top();
	auto& cmd = cl.cmds[frame.i];
	auto next_i = frame.i + 1;
	ivec3 new_frame = ivec3(-1);

	std::vector<Parameter> parameters;
	for (auto i = 0; i < std::get<1>(cmd).size(); i++)
	{
		auto& sp = std::get<1>(cmd)[i];
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
				auto ret = std::get<1>(cmd)[i++];
				assert(ret.type != Parameter::tExpression);
				if (ret.type == Parameter::tExternal)
				{
					ret.type = Parameter::tImmediate;
					auto& vec = external_parameters.at(ret.u.v.u);
					ret = vec.size() == 1 ? vec[0] : vec[lv - 1];
				}
				return ret;
			};
			i++;
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

	switch (std::get<0>(cmd))
	{
	case CommandList::cPrint:
		printf("Test\n");
		break;
	case CommandList::cStore:
		if (parameters.size() == 2)
		{
			void* src_ptr = nullptr;
			void* dst_ptr = nullptr;
			parameter_addr(parameters[0], src_ptr, ul);
			parameter_addr(parameters[1], dst_ptr, ul2);
			memcpy(dst_ptr, src_ptr, min(ul, ul2));
		}
		break;
	case CommandList::cBitInverse:
		if (parameters.size() == 1)
		{
			auto& v = variable_as.operator()<uint>(parameters[0].to_i());
			v = ~v;
		}
		break;
	case CommandList::cIfEqual:
	{
		auto end_i = next_i;
		if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
			end_i = it->second;

		if (parameters.size() == 2)
		{
			void* src_ptr = nullptr;
			void* dst_ptr = nullptr;
			parameter_addr(parameters[0], src_ptr, ul); ul2 = ul;
			parameter_addr(parameters[1], dst_ptr, ul);
			if (memcmp(src_ptr, dst_ptr, min(ul, ul2)) != 0) // jump to the end of the if block
				next_i = end_i + 1;
		}
		else
			next_i = end_i + 1;
	}
		break;
	case CommandList::cIfNotEqual:
	{
		auto end_i = next_i;
		if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
			end_i = it->second;

		if (parameters.size() == 2)
		{
			void* src_ptr = nullptr;
			void* dst_ptr = nullptr;
			parameter_addr(parameters[0], src_ptr, ul); ul2 = ul;
			parameter_addr(parameters[1], dst_ptr, ul);
			if (memcmp(src_ptr, dst_ptr, min(ul, ul2)) == 0) // jump to the end of the if block
				next_i = end_i + 1;
		}
		else
			next_i = end_i + 1;
	}
		break;
	case CommandList::cLoop:
		if (parameters.size() == 1)
		{
			new_frame.x = next_i;
			if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
			{
				new_frame.y = it->second;
				next_i = it->second + 1;
			}
			else
			{
				new_frame.y = next_i;
				next_i++;
			}
			new_frame.z = parameters[0].to_i();
		}
		break;
	case CommandList::cBreak:
		next_i = frame.end_i + 1;
		frame.loop_n = 0;
		break;
	case CommandList::cGenerateRnd:
		if (parameters.size() == 1)
			variable_as.operator()<uint>(parameters[0].to_i()) = rand();
		break;
	case CommandList::cRollDice100:
		if (parameters.size() == 1)
		{
			if (linearRand(0U, 99U) >= parameters[0].to_i()) // jump to the end of the block
			{
				if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
					next_i = it->second + 1;
				else
					next_i++;
			}
		}
		break;
	case CommandList::cGetNearbyCharacters:
		if (parameters.size() >= 2 && parameters.size() <= 4)
		{
			auto character_pos = character->node->pos;
			auto faction = parameters[0].to_i();
			auto search_range = parameters[1].to_f();
			auto start_radius = parameters.size() >= 3 ? parameters[2].to_f() : 0.f;
			auto central_angle = parameters.size() >= 4 ? parameters[3].to_f() : 0.f;
			auto direction_angle = central_angle > 0.f ? angle_xz(character_pos, target_pos) : 0.f;
			auto characters = find_characters(faction, character_pos, search_range, start_radius, central_angle, direction_angle);

			if (std::get<2>(cmd) != CommandList::vNull)
			{
				auto vec = variable_as.operator()<std::vector<lVariant>>(std::get<2>(cmd));
				vec.resize(characters.size());
				for (auto i = 0; i < vec.size(); i++)
					vec[i].p = characters[i];
			}
		}
		break;
	case CommandList::cNearestCharacter:
		if (parameters.size() >= 3 && parameters.size() <= 5)
		{
			auto character = variable_as.operator()<cCharacterPtr>(parameters[0].to_i());
			auto faction = variable_as.operator()<uint>(parameters[1].to_i());
			auto res = find_characters(faction, character->node->pos, parameters[2].to_f());
			cCharacterPtr ret = nullptr;
			auto marker = 0U; auto marker_time = 0.f;
			if (parameters.size() == 5)
			{
				marker = variable_as.operator()<uint>(parameters[3].to_i());
				marker_time = parameters[4].to_f();
			}
			for (auto c : res)
			{
				if (c != character)
				{
					if (marker && c->add_marker(marker, marker_time))
					{
						ret = c;
						break;
					}
				}
			}

			if (std::get<2>(cmd) != CommandList::vNull)
				variable_as.operator()<cCharacterPtr>(std::get<2>(cmd)) = ret;
		}
		break;
	case CommandList::cWait:
		if (parameters.size() == 1)
			wait_timer = parameters[0].to_f();
		break;
	case CommandList::cGetFaction:
		if (parameters.size() == 2)
			variable_as.operator()<uint>(parameters[1].to_i()) = variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->faction;
		break;
	case CommandList::cGetCharacterIDAndPos:
		if (parameters.size() == 2)
		{
			auto character = variable_as.operator()<cCharacterPtr>(parameters[0].to_i());
			auto& data = variable_as.operator()<IDAndPos>(parameters[1].to_i());
			data.id = character->object->uid;
			data.pos = character->node->pos;
		}
		break;
	case CommandList::cRestoreHP:
		if (parameters.size() == 1)
			character->restore_hp(parameters[0].to_i());
		break;
	case CommandList::cRestoreMP:
		if (parameters.size() == 1)
			character->restore_mp(parameters[0].to_i());
		break;
	case CommandList::cTakeDamage:
		if (parameters.size() == 2)
			character->take_damage((DamageType)parameters[0].to_i(), parameters[1].vt == Parameter::vPercentage ? character->hp_max * parameters[1].to_f() : parameters[1].to_i());
		break;
	case CommandList::cInflictDamage:
		if (parameters.size() == 4)
			variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->inflict_damage(variable_as.operator()<cCharacterPtr>(parameters[1].to_i()), (DamageType)parameters[2].to_i(), parameters[3].vt == Parameter::vPercentage ? character->hp_max * parameters[3].to_f() : parameters[3].to_i());
		break;
	case CommandList::cLevelUp:
		character->gain_exp(character->exp_max);
		break;
	case CommandList::cIncreaseHPMax:
		if (parameters.size() == 1)
		{
			if (parameters[0].vt == Parameter::vPercentage)
				character->hp_max *= 1.f + parameters[0].to_f();
			else
				character->hp_max += parameters[0].to_i();
		}
		break;
	case CommandList::cIncreaseMPMax:
		if (parameters.size() == 1)
		{
			if (parameters[0].vt == Parameter::vPercentage)
				character->mp_max *= 1.f + parameters[0].to_f();
			else
				character->mp_max += parameters[0].to_i();
		}
		break;
	case CommandList::cIncreaseATK:
		if (parameters.size() == 1)
		{
			if (parameters[0].vt == Parameter::vPercentage)
				character->atk *= 1.f + parameters[0].to_f();
			else
				character->atk += parameters[0].to_i();
		}
		break;
	case CommandList::cIncreasePHYDEF:
		if (parameters.size() == 1)
			character->phy_def += parameters[0].to_i();
		break;
	case CommandList::cIncreaseMAGDEF:
		if (parameters.size() == 1)
			character->mag_def += parameters[0].to_i();
		break;
	case CommandList::cIncreaseHPREG:
		if (parameters.size() == 1)
			character->hp_reg += parameters[0].to_i();
		break;
	case CommandList::cIncreaseMPREG:
		if (parameters.size() == 1)
			character->mp_reg += parameters[0].to_i();
		break;
	case CommandList::cIncreaseMOVSP:
		if (parameters.size() == 1)
			character->mov_sp += parameters[0].to_i();
		break;
	case CommandList::cIncreaseATKSP:
		if (parameters.size() == 1)
			character->atk_sp += parameters[0].to_i();
		break;
	case CommandList::cAddBuff:
		if (parameters.size() >= 3 && parameters.size() <= 5)
			variable_as.operator()<cCharacterPtr>(parameters[0].to_i())->add_buff(parameters[1].to_i(), parameters[2].to_f(), parameters.size() >= 4 ? parameters[3].to_i() : 0, parameters.size() >= 5 ? parameters[4].to_i() : false);
		break;
	case CommandList::cAddAttackEffect:
		if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
		{
			add_callback(next_i, it->second, character->attack_effects.emplace_back());
			next_i = it->second + 1;
		}
		else
		{
			add_callback(next_i, next_i, character->attack_effects.emplace_back());
			next_i++;
		}
		break;
	case CommandList::cTeleportToTarget:
		teleport(character, target_character ? target_character->node->pos : target_pos);
		break;
	case CommandList::cSendMessage:
		if (parameters.size() == 3)
		{
			auto comp = variable_as.operator()<ComponentPtr>(parameters[0].to_i());
			void* ptr = nullptr;
			variable_addr(parameters[2].to_i(), ptr, ul);
			comp->send_message(parameters[1].to_u(), ptr, ul);
		}
		break;
	case CommandList::cSetSectorCollideCallback:
	{
		auto end_i = next_i;
		if (auto it = cl.sub_groups.find(next_i); it != cl.sub_groups.end())
			end_i = it->second;

		if (parameters.size() == 1)
		{
			if (auto entity = variable_as.operator()<EntityPtr>(parameters[0].to_i()); entity)
			{
				if (auto collider = entity->get_component_t<cSectorCollider>(); collider)
				{
					collider->faction = ~character->faction;
					collider->host = character;
					add_callback(next_i, end_i, collider->callback);
				}
			}
		}

		next_i = end_i + 1;
	}
		break;
	case CommandList::cAddEffect:
		if (parameters.size() == 2)
		{
			auto ret = add_effect(parameters[0].to_i(), character->node->pos, vec3(0.f), parameters[1].to_f());
			if (std::get<2>(cmd) != CommandList::vNull)
				variable_as.operator()<voidptr>(std::get<2>(cmd)) = ret;
		}
		break;
	case CommandList::cAddEffectFaceTarget:
		if (parameters.size() == 2)
		{
			auto ret = add_effect(parameters[0].to_i(), character->node->pos + vec3(0.f, character->nav_agent->height * 0.5f, 0.f), vec3(angle_xz(character->node->pos, target_pos), 0.f, 0.f), parameters[1].to_f());
			if (std::get<2>(cmd) != CommandList::vNull)
				variable_as.operator()<voidptr>(std::get<2>(cmd)) = ret;
		}
		break;
	}

	if (next_i > frame.end_i)
	{
		if (frame.loop_n > 0)
		{
			frame.loop_n--;
			frame.i = frame.beg_i;
		}
		else
			frames.pop();
	}
	else
		frame.i = next_i;

	if (new_frame.x != -1 && new_frame.z > 0)
	{
		auto& frame = frames.emplace();
		frame.beg_i = new_frame.x;
		frame.end_i = new_frame.y;
		frame.i = new_frame.x;
		frame.loop_n = new_frame.z - 1;
	}
}

std::list<CommandListExecuteThread> cl_threads;
