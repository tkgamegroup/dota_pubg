
ItemShortcut::ItemShortcut(ItemInstance* ins) :
	ins(ins)
{
	type = tItem;
	id = ins->id;
}

void ItemShortcut::draw(ImDrawList* dl, const vec2& p0, const vec2& p1)
{
	auto& item = Item::get(ins->id);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(item.name.c_str());
		ImGui::TextUnformatted(item.description.c_str());
		ImGui::EndTooltip();
	}
	dl->AddImage(item.icon_image, p0, p1, item.icon_uvs.xy(), item.icon_uvs.zw());
}

void ItemShortcut::click()
{
	main_player.character->use_item(ins);
}

AbilityShortcut::AbilityShortcut(AbilityInstance* ins) :
	ins(ins)
{
	type = tAbility;
	id = ins->id;
}

void AbilityShortcut::draw(ImDrawList* dl, const vec2& p0, const vec2& p1)
{
	auto& ability = Ability::get(ins->id);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::TextUnformatted(ability.name.c_str());
		ImGui::TextUnformatted(ability.description.c_str());
		ImGui::EndTooltip();
	}
	dl->AddImage(ability.icon_image, p0, p1, ability.icon_uvs.xy(), ability.icon_uvs.zw());

	if (ins->cd_max > 0.f && ins->cd_timer > 0.f)
	{
		dl->PushClipRect(p0, p1);
		auto c = (p0 + p1) * 0.5f;
		dl->PathLineTo(c);
		dl->PathArcTo(c, p1.x - p0.x, ((ins->cd_timer / ins->cd_max) * 2.f - 0.5f) * glm::pi<float>(), -0.5f * glm::pi<float>());
		dl->PathFillConvex(ImColor(0.f, 0.f, 0.f, 0.5f));
		dl->AddText(p0 + vec2(8.f), ImColor(1.f, 1.f, 1.f, 1.f), std::format("{:.1f}", ins->cd_timer).c_str());
		dl->PopClipRect();
	}
}

void AbilityShortcut::click()
{
	if (ins->cd_timer > 0.f)
	{
		illegal_op_str = "Cooldowning.";
		illegal_op_str_timer = 3.f;
		return;
	}
	auto& ability = Ability::get(ins->id);
	if (main_player.character->mp < ability.get_mp(ins->lv))
	{
		illegal_op_str = "Not Enough MP.";
		illegal_op_str_timer = 3.f;
		return;
	}
	select_mode = ability.target_type;
	if (select_mode == TargetNull)
	{
		if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
			new CharacterCommandCastAbility(main_player.character, ins);
		else if (multi_player == MultiPlayerAsClient)
		{
			std::ostringstream res;
			nwCommandCharacterStruct stru;
			stru.id = main_player.character->object->uid;
			stru.type = "CastAbility"_h;
			stru.id2 = ins->id;
			pack_msg(res, nwCommandCharacter, stru);
			so_client->send(res.str());
		}
	}
	else
	{
		if (ability.target_type & TargetLocation)
		{
			select_location_callback = [this](const vec3& location) {
				if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
					new CharacterCommandCastAbilityToLocation(main_player.character, ins, location);
				else if (multi_player == MultiPlayerAsClient)
				{
					std::ostringstream res;
					nwCommandCharacterStruct stru;
					stru.id = main_player.character->object->uid;
					stru.type = "CastAbilityToLocation"_h;
					stru.id2 = ins->id;
					stru.t.location = location;
					pack_msg(res, nwCommandCharacter, stru);
					so_client->send(res.str());
				}
			};
			select_distance = ability.get_distance(ins->lv);
			select_range = ability.get_range(ins->lv);
			select_angle = ability.angle;
			select_start_radius = ability.start_radius;
		}
		if (ability.target_type & TargetEnemy)
		{
			select_enemy_callback = [this](cCharacterPtr character) {
				if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
					new CharacterCommandCastAbilityToTarget(main_player.character, ins, character);
				else if (multi_player == MultiPlayerAsClient)
				{
					std::ostringstream res;
					nwCommandCharacterStruct stru;
					stru.id = main_player.character->object->uid;
					stru.type = "CastAbilityToTarget"_h;
					stru.id2 = ins->id;
					stru.t.target = character->object->uid;
					pack_msg(res, nwCommandCharacter, stru);
					so_client->send(res.str());
				}
			};
			select_distance = ability.get_distance(ins->lv);
		}
	}
}

std::unique_ptr<Shortcut> shortcuts[10];