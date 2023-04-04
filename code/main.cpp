
//
//std::string illegal_op_str;
//float illegal_op_str_timer = 0.f;
//
//std::string tooltip;
//
//Tracker<cCharacterPtr> focus_character;
//
//cMain::~cMain()
//{
//	deinit_vision();
//}
//
//static std::vector<std::pair<uint, vec3>> location_tips;
//void add_location_icon(const vec3& pos)
//{
//	location_tips.emplace_back(30, pos);
//}
//
//void toggle_equipment_view()
//{
//	if (!view_equipment.opened)
//		view_equipment.open();
//	else
//		view_equipment.close();
//}
//
//void toggle_ability_view()
//{
//	if (!view_ability.opened)
//		view_ability.open();
//	else
//		view_ability.close();
//}
//
//void toggle_inventory_view()
//{
//	if (!view_inventory.opened)
//		view_inventory.open();
//	else
//		view_inventory.close();
//}
//
//void toggle_settings_view()
//{
//	if (!view_settings.opened)
//		view_settings.open();
//	else
//		view_settings.close();
//}
//
//float gtime = -1.f;
//
//struct MonsterSpawnningRule
//{
//	uint prefab_id;
//	float delay;
//	float number_function_factor_a;
//	float number_function_factor_b;
//	float number_function_factor_c;
//
//	uint spawnned_numbers = 0;
//};
//std::vector<MonsterSpawnningRule> monster_spawnning_rules;
//
//void init_spawnning_rules()
//{
//	for (auto& section : parse_ini_file(Path::get(L"assets\\monster_spawnnings.ini")).sections)
//	{
//		auto prefab_id = CharacterPreset::find(section.name);
//		if (prefab_id != -1)
//		{
//			auto& rule = monster_spawnning_rules.emplace_back();
//			rule.prefab_id = prefab_id;
//			for (auto& e : section.entries)
//			{
//				switch (e.key_hash)
//				{
//				case "delay"_h:
//					rule.delay = s2t<float>(e.values[0]);
//					break;
//				case "number_function_factor_a"_h:
//					rule.number_function_factor_a = s2t<float>(e.values[0]);
//					break;
//				case "number_function_factor_b"_h:
//					rule.number_function_factor_b = s2t<float>(e.values[0]);
//					break;
//				case "number_function_factor_c"_h:
//					rule.number_function_factor_c = s2t<float>(e.values[0]);
//					break;
//				}
//			}
//		}
//	}
//}
//
//void cMain::on_active()
//{
//	graphics::gui_set_current();
//	graphics::gui_callbacks.add([this]() {
//
//		ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_ext.x * 0.5f, tar_ext.y), ImGuiCond_Always, ImVec2(0.5f, 1.f));
//		ImGui::SetNextWindowSize(ImVec2(600.f, 42.f), ImGuiCond_Always);
//		ImGui::Begin("##main_panel", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
//			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
//		if (main_player.character)
//		{
//			ImGui::BeginGroup();
//			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.f, 1.f));
//			auto dl = ImGui::GetWindowDrawList();
//			const auto icon_size = 32.f;
//			for (auto i = 0; i < countof(shortcuts); i++)
//			{
//				if (i > 0) ImGui::SameLine();
//				auto pressed = ImGui::InvisibleButton(("shortcut" + str(i)).c_str(), ImVec2(icon_size, icon_size));
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p1, ImColor(0.f, 0.f, 0.f, 0.5f));
//				dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
//
//				auto shortcut = shortcuts[i].get();
//				shortcut->draw(dl, p0, p1);
//
//				if (ImGui::BeginDragDropSource())
//				{
//					ImGui::SetDragDropPayload("shortcut", &i, sizeof(int));
//					ImGui::EndDragDropSource();
//				}
//
//				if (pressed)
//					shortcut->click();
//
//				if (ImGui::BeginDragDropTarget())
//				{
//					if (auto payload = ImGui::AcceptDragDropPayload("shortcut"); payload)
//					{
//						auto j = *(int*)payload->Data;
//						if (i != j)
//						{
//							std::swap(shortcuts[i]->key, shortcuts[j]->key);
//							std::swap(shortcuts[i], shortcuts[j]);
//						}
//					}
//					if (auto payload = ImGui::AcceptDragDropPayload("ability"); payload)
//					{
//						auto j = *(int*)payload->Data;
//						auto key = shortcut->key;
//						shortcut = new AbilityShortcut(main_player.character->abilities[j].get());
//						shortcut->key = key;
//						shortcuts[i].reset(shortcut);
//					}
//					ImGui::EndDragDropTarget();
//				}
//				if (shortcut && shortcut->key != KeyboardKey_Count)
//					dl->AddText(p0 + vec2(4.f, 0.f), ImColor(1.f, 1.f, 1.f), TypeInfo::serialize_t(shortcut->key).c_str());
//			}
//			ImGui::PopStyleVar();
//			ImGui::EndGroup();
//
//			ImGui::SameLine();
//			ImGui::BeginGroup();
//			{
//				static auto img = graphics::Image::get("assets\\icons\\head.png");
//				auto pressed = ImGui::InvisibleButton("btn_equipment", ImVec2(icon_size, icon_size));
//				auto hovered = ImGui::IsItemHovered();
//				auto active = ImGui::IsItemActive();
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
//				dl->AddImage(img, p0, p1);
//				if (pressed)
//					toggle_equipment_view();
//			}
//			ImGui::SameLine();
//			{
//				static auto img = graphics::Image::get("assets\\icons\\book.png");
//				auto pressed = ImGui::InvisibleButton("btn_ability", ImVec2(icon_size, icon_size));
//				auto hovered = ImGui::IsItemHovered();
//				auto active = ImGui::IsItemActive();
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
//				dl->AddImage(img, p0, p1);
//				if (pressed)
//					toggle_ability_view();
//			}
//			ImGui::SameLine();
//			{
//				static auto img = graphics::Image::get("assets\\icons\\bag.png");
//				auto pressed = ImGui::InvisibleButton("btn_bag", ImVec2(icon_size, icon_size));
//				auto hovered = ImGui::IsItemHovered();
//				auto active = ImGui::IsItemActive();
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
//				dl->AddImage(img, p0, p1);
//				if (pressed)
//					toggle_inventory_view();
//			}
//			ImGui::SameLine();
//			{
//				static auto img = graphics::Image::get("assets\\icons\\gear.png");
//				auto pressed = ImGui::InvisibleButton("btn_settings", ImVec2(icon_size, icon_size));
//				auto hovered = ImGui::IsItemHovered();
//				auto active = ImGui::IsItemActive();
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p1, active ? ImColor(0.f, 0.1f, 0.3f, 1.f) : (hovered ? ImColor(0.f, 0.2f, 0.5f, 1.f) : ImColor(0.f, 0.2f, 0.5f, 0.5f)));
//				dl->AddImage(img, p0, p1);
//				if (pressed)
//					toggle_settings_view();
//			}
//			ImGui::EndGroup();
//		}
//		ImGui::End();
//
//		auto show_buffs = [](ImDrawList* dl, cCharacterPtr character) {
//			for (auto i = 0; i < character->buffs.size(); i++)
//			{
//				if (i > 0) ImGui::SameLine();
//				auto& ins = character->buffs[i];
//				auto& buff = Buff::get(ins->id);
//				ImGui::Dummy(ImVec2(16, 16));
//				if (ImGui::IsItemHovered())
//				{
//					ImGui::BeginTooltip();
//					ImGui::TextUnformatted(buff.name.c_str());
//					ImGui::Text("%d s", (int)ins->timer);
//					ImGui::EndTooltip();
//				}
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddImage(buff.icon_image, p0, p1, buff.icon_uvs.xy(), buff.icon_uvs.zw());
//			}
//		};
//
//		auto hp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
//			ImGui::Dummy(ImVec2(width, height));
//			auto p0 = (vec2)ImGui::GetItemRectMin();
//			auto p1 = (vec2)ImGui::GetItemRectMax();
//			dl->AddRectFilled(p0, p0 + vec2((float)character->hp / (float)character->hp_max * width, height), ImColor(0.3f, 0.7f, 0.2f));
//			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
//			auto str = std::format("{}/{}", character->hp, character->hp_max);
//			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
//			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
//		};
//		auto mp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
//			ImGui::Dummy(ImVec2(width, height));
//			auto p0 = (vec2)ImGui::GetItemRectMin();
//			auto p1 = (vec2)ImGui::GetItemRectMax();
//			dl->AddRectFilled(p0, p0 + vec2((float)character->mp / (float)character->mp_max * width, height), ImColor(0.2f, 0.3f, 0.7f));
//			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
//			auto str = std::format("{}/{}", character->mp, character->mp_max);
//			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
//			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
//		};
//		auto exp_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
//			ImGui::Dummy(ImVec2(width, height));
//			auto p0 = (vec2)ImGui::GetItemRectMin();
//			auto p1 = (vec2)ImGui::GetItemRectMax();
//			dl->AddRectFilled(p0, p0 + vec2((float)character->exp / (float)character->exp_max * width, height), ImColor(0.7f, 0.7f, 0.2f));
//			dl->AddRect(p0, p1, ImColor(0.7f, 0.7f, 0.7f));
//			auto str = std::format("LV {}  {}/{}", character->lv, character->exp, character->exp_max);
//			auto text_width = ImGui::CalcTextSize(str.c_str()).x;
//			dl->AddText(p0 + vec2((width - text_width) * 0.5f, 0.f), ImColor(1.f, 1.f, 1.f), str.c_str());
//		};
//		auto action_bar = [](ImDrawList* dl, float width, float height, cCharacterPtr character) {
//			float time = 0.f;
//			if (character->attack_timer > 0.f)
//				time = character->attack_timer;
//			else if (character->cast_timer > 0.f)
//				time = character->cast_timer;
//			if (time > 0.f)
//			{
//				ImGui::Dummy(ImVec2(width, height));
//				auto p0 = (vec2)ImGui::GetItemRectMin();
//				auto p1 = (vec2)ImGui::GetItemRectMax();
//				dl->AddRectFilled(p0, p0 + vec2((1.f - fract(time)) * width, height), ImColor(0.7f, 0.7f, 0.7f));
//				dl->AddRect(p0, p1, ImColor(0.3f, 0.3f, 0.3f));
//				dl->AddText(vec2((p0.x + p1.x) * 0.5f - 3.f, p0.y - 7.f), ImColor(1.f, 1.f, 1.f), str((int)time).c_str());
//			}
//		};
//
//		if (main_player.character)
//		{
//			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(8.f, 4.f), ImGuiCond_Always, ImVec2(0.f, 0.f));
//			ImGui::SetNextWindowSize(ImVec2(200.f, 120.f), ImGuiCond_Always);
//			ImGui::Begin("##main_player", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
//				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
//
//			auto dl = ImGui::GetWindowDrawList();
//			const auto bar_width = ImGui::GetContentRegionAvail().x;
//			const auto bar_height = 16.f;
//			hp_bar(dl, bar_width, bar_height, main_player.character);
//			mp_bar(dl, bar_width, bar_height, main_player.character);
//			exp_bar(dl, bar_width, bar_height, main_player.character);
//			show_buffs(dl, main_player.character);
//			action_bar(dl, bar_width, 4.f, main_player.character);
//			auto pos = main_player.character->node->pos;
//			ImGui::Text("%d, %d", (int)pos.x, (int)pos.z);
//
//			ImGui::End();
//		}
//
//		if (focus_character.obj)
//		{
//			ImGui::SetNextWindowPos(sInput::instance()->offset + vec2(tar_ext.x - 8.f, 4.f), ImGuiCond_Always, ImVec2(1.f, 0.f));
//			ImGui::SetNextWindowSize(ImVec2(100.f, 100.f), ImGuiCond_Always);
//			ImGui::Begin("##focus_character", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
//				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
//
//			auto dl = ImGui::GetWindowDrawList();
//			const auto bar_width = ImGui::GetContentRegionAvail().x;
//			const auto bar_height = 16.f;
//			hp_bar(dl, bar_width, bar_height, focus_character.obj);
//			mp_bar(dl, bar_width, bar_height, focus_character.obj);
//
//			show_buffs(dl, focus_character.obj);
//			action_bar(dl, bar_width, 4.f, focus_character.obj);
//
//			ImGui::End();
//		}
//
//		if (!tooltip.empty())
//		{
//			auto dl = ImGui::GetBackgroundDrawList();
//			ImGui::BeginTooltip();
//			ImGui::TextUnformatted(tooltip.c_str());
//			ImGui::EndTooltip();
//		}
//
//		if (main_camera.camera)
//		{
//			if (!floating_tips.empty())
//			{
//				for (auto& t : floating_tips)
//				{
//					auto p = main_camera.camera->world_to_screen(t.pos);
//					if (p.x > 0.f && p.y > 0.f)
//					{
//						p.xy += sInput::instance()->offset;
//						auto dl = ImGui::GetBackgroundDrawList();
//						if (t.size.x == 0.f || t.size.y == 0.f)
//							t.size = ImGui::CalcTextSize(t.text.c_str());
//						dl->AddText(p - vec2(t.size.x * 0.5f, 0.f), ImColor(t.color.r, t.color.g, t.color.b, 255), t.text.c_str());
//					}
//					t.pos.y += 1.8f * delta_time;
//				}
//				for (auto it = floating_tips.begin(); it != floating_tips.end();)
//				{
//					it->ticks--;
//					if (it->ticks == 0)
//						it = floating_tips.erase(it);
//					else
//						it++;
//				}
//			}
//
//			if (!location_tips.empty())
//			{
//				static graphics::ImagePtr icon_location = nullptr;
//				if (!icon_location)
//					icon_location = graphics::Image::get(L"assets\\icons\\location.png");
//				for (auto& t : location_tips)
//				{
//					auto p = main_camera.camera->world_to_screen(t.second);
//					if (p.x > 0.f && p.y > 0.f)
//					{
//						p.xy += sInput::instance()->offset;
//						auto dl = ImGui::GetBackgroundDrawList();
//						auto ext = (vec2)icon_location->extent;
//						dl->AddImage(icon_location, p - vec2(ext.x * 0.5f, ext.y), p + vec2(ext.x * 0.5f, 0.f), vec2(0.f), vec2(1.f), ImColor(1.f, 1.f, 1.f, max(0.f, t.first / 30.f)));
//					}
//				}
//				for (auto it = location_tips.begin(); it != location_tips.end();)
//				{
//					it->first--;
//					if (it->first == 0)
//						it = location_tips.erase(it);
//					else
//						it++;
//				}
//			}
//		}
//
//		if (illegal_op_str_timer > 0.f)
//		{
//			auto dl = ImGui::GetForegroundDrawList();
//			auto text_size = (vec2)ImGui::CalcTextSize(illegal_op_str.c_str());
//			auto p = vec2((tar_ext.x - text_size.x) * 0.5f, tar_ext.y - 160.f - text_size.y);
//			p.xy += sInput::instance()->offset;
//			auto alpha = 1.f;
//			if (illegal_op_str_timer < 1.f)
//				alpha *= mix(0.f, 1.f, illegal_op_str_timer);
//			auto border = 0.f;
//			if (illegal_op_str_timer > 2.9f)
//				border = mix(8.f, 0.f, (illegal_op_str_timer - 2.9f) / 0.1f);
//			else if (illegal_op_str_timer > 2.5f)
//				border = mix(0.f, 8.f, (illegal_op_str_timer - 2.5f) / 0.4f);
//			dl->AddRectFilled(p - vec2(2.f + border), p + text_size + vec2(2.f + border), ImColor(1.f, 0.f, 0.f, 0.5f * alpha));
//			dl->AddText(p, ImColor(1.f, 1.f, 1.f, 1.f * alpha), illegal_op_str.c_str());
//			illegal_op_str_timer -= delta_time;
//		}
//
//		{
//			static graphics::ImagePtr icon_cursors = nullptr;
//			if (!icon_cursors)
//				icon_cursors = graphics::Image::get(L"assets\\icons\\rpg_cursor_set.png");
//			auto tiles = vec2(icon_cursors->tiles);
//			int cursor_x = 0, cursor_y = 0;
//			if (select_mode != TargetNull)
//			{
//				cursor_x = 3;
//				cursor_y = 0;
//			}
//			auto pos = sInput::instance()->mpos + sInput::instance()->offset;
//			auto dl = ImGui::GetForegroundDrawList();
//			dl->AddImage(icon_cursors, pos + vec2(-32.f), pos + vec2(32.f),
//				vec2(cursor_x, cursor_y) / tiles,
//				vec2(cursor_x + 1, cursor_y + 1) / tiles);
//		}
//	}, (uint)this);
//	graphics::gui_cursor_callbacks.add([this](CursorType cursor) {
//		auto mpos = sInput::instance()->mpos;
//		auto screen_ext = sRenderer::instance()->target_extent();
//		if (mpos.x < 0.f || mpos.x > screen_ext.x || mpos.y < 0.f || mpos.y > screen_ext.y)
//			return cursor;
//		return CursorNone;
//	}, (uint)this);
//}
//
//void cMain::on_inactive()
//{
//	node->drawers.remove("main"_h);
//
//	graphics::gui_callbacks.remove((uint)this);
//	graphics::gui_cursor_callbacks.remove((uint)this);
//}
//
//void cMain::start()
//{
//	printf("main started\n");
//	srand(time(0));
//	root = entity;
//
//	for (auto i = 0; i < countof(shortcuts); i++)
//	{
//		auto shortcut = new Shortcut;
//		shortcut->key = KeyboardKey(Keyboard_1 + i);
//		shortcuts[i].reset(shortcut);
//	}
//
//	main_camera.init(entity->find_child("Camera"));
//	main_terrain.init(entity->find_child("terrain"));
//
//	init_vision();
//
//	init_effects();
//	init_projectiles();
//	init_items();
//	init_buffs();
//	init_abilities();
//	init_characters();
//	init_spawnning_rules();
//	for (auto& preset : effect_presets)
//	{
//		get_prefab(preset.path)->forward_traversal([](EntityPtr e) {
//			if (auto particle_system = e->get_component_t<cParticleSystem>(); particle_system)
//				sRenderer::instance()->get_material_res(particle_system->material);
//		});
//	}
//
//	if (auto nav_scene = entity->get_component_t<cNavScene>(); nav_scene)
//	{
//		nav_scene->finished_callback.add([this]() {
//			if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
//			{
//				vec3 player1_coord;
//				uint player1_faction;
//				uint player1_preset_id;
//				add_player(player1_coord, player1_faction, player1_preset_id);
//				auto character = add_character(player1_preset_id, player1_coord, player1_faction);
//				main_player.faction = player1_faction;
//				main_player.character_id = character->object->uid;
//				main_player.init(character->entity);
//				if (auto harvester = main_player.entity->get_component_t<cNWDataHarvester>(); harvester)
//				{
//					//harvester->add_target("exp"_h);
//					//harvester->add_target("exp_max"_h);
//					//harvester->add_target("atk_type"_h);
//					//harvester->add_target("atk"_h);
//					//harvester->add_target("phy_def"_h);
//					//harvester->add_target("mag_def"_h);
//					//harvester->add_target("hp_reg"_h);
//					//harvester->add_target("mp_reg"_h);
//					//harvester->add_target("mov_sp"_h);
//					//harvester->add_target("atk_sp"_h);
//				}
//
//				if (main_terrain.hf_terrain && !main_terrain.site_positions.empty())
//				{
//					add_chest(player1_coord + vec3(-3.f, 0.f, 3.f), Item::find("Magic Candy"));
//					add_chest(player1_coord + vec3(-2.f, 0.f, 3.f), Item::find("Magic Candy"));
//					add_chest(player1_coord + vec3(-1.f, 0.f, 3.f), Item::find("Magic Candy"));
//
//					//for (auto i = 1; i < main_terrain.site_centrality.size() - 1; i++)
//					//{
//					//	auto coord = main_terrain.get_coord_by_centrality(i);
//
//						//static uint preset_ids[] = {
//						//	CharacterPreset::find("Life Stealer"),
//						//	CharacterPreset::find("Slark")
//						//};
//
//					//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
//					//	new CharacterCommandAttackLocation(character, coord);
//					//}
//					//for (auto i = 0; i < 100; i++)
//					//{
//					//	auto coord = main_terrain.get_coord(vec2(linearRand(0.f, 1.f), linearRand(0.f, 1.f)));
//
//						//static uint preset_ids[] = {
//						//	CharacterPreset::find("Spiderling"),
//						//	CharacterPreset::find("Treant"),
//						//	CharacterPreset::find("Boar")
//						//};
//
//					//	auto character = add_character(preset_ids[linearRand(0U, (uint)countof(preset_ids) - 1)], coord, FactionCreep);
//					//	character->entity->add_component<cAI>();
//					//}
//				}
//
//				gtime = 0.f;
//			}
//		});
//	}
//}
//
//void cMain::update()
//{
//	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
//	{
//		for (auto it = cl_threads.begin(); it != cl_threads.end();)
//		{
//			auto& thread = *it;
//			if (thread.wait_timer > 0.f)
//			{
//				thread.wait_timer -= delta_time;
//				it++;
//			}
//			else
//			{
//				while (!thread.frames.empty() && thread.wait_timer <= 0.f)
//					thread.execute();
//				if (thread.frames.empty())
//					it = cl_threads.erase(it);
//				else
//					it++;
//			}
//		}
//
//		// auto attack
//		if (main_player.character)
//		{
//			if (main_player.character->command && main_player.character->command->type == "Idle"_h)
//				new CharacterCommandHold(main_player.character);
//		}
//	}
//
//	switch (multi_player)
//	{
//	case MultiPlayerAsHost:
//		nw_mtx.lock();
//		if (!nw_msgs.empty())
//		{
//			auto p = nw_msgs.data();
//			auto e = p + nw_msgs.size();
//			while (p < e)
//			{
//				auto msg = *(uchar*)p;
//				p += sizeof(uchar);
//				switch (msg)
//				{
//				case nwCommandCharacter:
//				{
//					auto& stru = *(nwCommandCharacterStruct*)p;
//					auto it = objects.find(stru.id);
//					if (it == objects.end())
//						continue;
//					auto character = it->second->entity->get_component_t<cCharacter>();
//					switch (stru.type)
//					{
//					case "Idle"_h:
//						new CharacterCommandIdle(character);
//						break;
//					case "MoveTo"_h:
//						new CharacterCommandMoveTo(character, stru.t.location);
//						break;
//					case "AttackTarget"_h:
//						if (auto it = objects.find(stru.t.target); it != objects.end())
//							new CharacterCommandAttackTarget(character, it->second->entity->get_component_t<cCharacter>());
//						break;
//					case "AttackLocation"_h:
//						new CharacterCommandAttackLocation(character, stru.t.location);
//						break;
//					case "PickUp"_h:
//						if (auto it = objects.find(stru.t.target); it != objects.end())
//							new CharacterCommandPickUp(character, it->second->entity->get_component_t<cChest>());
//						break;
//					case "CastAbility"_h:
//						break;
//					}
//					p += sizeof(nwCommandCharacterStruct);
//				}
//					break;
//				}
//			}
//			nw_msgs.clear();
//		}
//		nw_mtx.unlock();
//		break;
//	case MultiPlayerAsClient:
//		nw_mtx.lock();
//		if (!nw_msgs.empty())
//		{
//			auto p = nw_msgs.data();
//			auto e = p + nw_msgs.size();
//			while (p < e)
//			{
//				auto msg = *(uchar*)p;
//				p += sizeof(uchar);
//				switch (msg)
//				{
//				case nwNewPlayerInfo:
//				{
//					auto& stru = *(nwNewPlayerInfoStruct*)p;
//					main_player.faction = stru.faction;
//					main_player.character_id = stru.character_id;
//					p += sizeof(nwNewPlayerInfoStruct);
//				}
//					break;
//				case nwAddObjects:
//				{
//					nwAddObjectsStruct stru;
//					unserialize_binary([&](void* data, uint size) {
//						memcpy(data, p, size);
//						p += size;
//					}, &stru);
//					for (auto& item : stru.items)
//					{
//						if (item.prefab_id < 2000)
//						{
//							auto character = add_character(item.prefab_id - 1000, vec3(0.f, -1000.f, 0.f), 0, item.id);
//							character->entity->children[0]->set_enable(false); 
//							if (item.id == main_player.character_id)
//								main_player.init(character->entity);
//						}
//						else if (item.prefab_id < 3000)
//						{
//							auto projectle = add_projectile(item.prefab_id - 2000, vec3(0.f, -1000.f, 0.f), nullptr, 0.f, nullptr, item.id);
//							projectle->entity->children[0]->set_enable(false);
//						}
//						else if (item.prefab_id < 4000)
//						{
//							auto projectle = add_projectile(item.prefab_id - 3000, vec3(0.f, -1000.f, 0.f), nullptr, 0.f, nullptr, item.id);
//							projectle->entity->children[0]->set_enable(false);
//						}
//						else
//						{
//							auto chest = add_chest(vec3(0.f, -1000.f, 0.f), -1, 0, item.id);
//							chest->entity->children[0]->set_enable(false);
//						}
//					}
//				}
//					break;
//				case nwRemoveObjects:
//				{
//					nwRemoveObjectsStruct stru;
//					unserialize_binary([&](void* data, uint size) {
//						memcpy(data, p, size);
//						p += size;
//					}, & stru);
//					for (auto id : stru.ids)
//					{
//						auto it = objects.find(id);
//						if (it == objects.end())
//							continue;
//						auto entity = it->second->entity;
//						add_event([entity]() {
//							entity->remove_from_parent();
//							return false;
//						});
//					}
//				}
//					break;
//				case nwUpdateObjects:
//				{
//					nwUpdateObjectsStruct stru;
//					unserialize_binary([&](void* data, uint size) {
//						memcpy(data, p, size);
//						p += size;
//					}, &stru);
//					for (auto& item : stru.items)
//					{
//						auto it = objects.find(item.obj_id);
//						if (it == objects.end())
//							continue;
//						auto entity = it->second->entity;
//						for (auto& citem : item.comps)
//						{
//							auto comp = entity->components[citem.idx].get();
//							auto ui = find_udt(comp->type_hash);
//							auto p = citem.datas.data();
//							for (auto i = 0; i < citem.names.size(); i++)
//							{
//								auto name = citem.names[i];
//								auto vi = ui->find_variable(name); auto len = vi->type->size;
//								auto dst = (char*)comp + vi->offset;
//								memcpy(dst, p, len);
//								p += len;
//
//								if (name == "visible_flags"_h)
//									entity->children[0]->set_enable(*(uint*)dst & main_player.faction);
//							}
//						}
//					}
//				}
//					break;
//				}
//			}
//			nw_msgs.clear();
//		}
//		nw_mtx.unlock();
//		break;
//	}
//
//	// move camera smoothly
//	if (main_camera.node && main_player.node)
//	{
//		static vec3 velocity(0.f);
//		main_camera.node->set_eul(vec3(0.f, -camera_angle, 0.f));
//		main_camera.node->set_pos(smooth_damp(main_camera.node->pos,
//			main_player.node->pos + camera_length * main_camera.node->g_rot[2], velocity, 0.3f, 10000.f, delta_time));
//	}
//
//	update_vision();
//
//	if (multi_player == SinglePlayer || multi_player == MultiPlayerAsHost)
//	{
//		if (main_player.character)
//		{
//			if (gtime >= 0.f)
//			{
//				gtime += delta_time;
//				for (auto& rule : monster_spawnning_rules)
//				{
//					auto t = gtime / 60.f;
//					t -= rule.delay;
//					if (t < 0.f)
//						continue;
//
//					auto n = rule.number_function_factor_a * t + rule.number_function_factor_b * t * t + rule.number_function_factor_c;
//					n -= rule.spawnned_numbers;
//
//					for (auto i = 0; i < n; i++)
//					{
//						auto uv = (main_player.node->pos.xz() + circularRand(20.f)) / main_terrain.extent.xz();
//						if (uv.x > 0.f && uv.x < 1.f && uv.y > 0.f && uv.y < 1.f)
//						{
//							auto pos = main_terrain.get_coord(uv);
//							//if (pos.y > main_terrain.node->pos.y + 3.f)
//							{
//								auto path = sScene::instance()->query_navmesh_path(pos, main_player.node->pos, 0);
//								if (path.size() >= 2 && distance(path.back(), main_player.node->pos) < 0.3f)
//								{
//									auto character = add_character(rule.prefab_id, pos, FactionCreep);
//									character->add_buff(Buff::find("Cursed"), -1.f, uint(gtime / 60.f) + 1);
//									new CharacterCommandAttackTarget(character, main_player.character);
//
//									rule.spawnned_numbers++;
//								}
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//
//	if (multi_player == MultiPlayerAsHost)
//	{
//		if (!nw_new_players.empty())
//		{
//			for (auto so_id : nw_new_players)
//			{
//				vec3 pos;
//				uint faction;
//				uint prefab_id;
//				add_player(pos, faction, prefab_id);
//				auto character = add_character(prefab_id, pos, faction);
//
//				nw_players[faction].push_back(so_id);
//
//				std::ostringstream res;
//				{
//					nwNewPlayerInfoStruct stru;
//					stru.faction = faction;
//					stru.character_id = character->object->uid;
//					pack_msg(res, nwNewPlayerInfo, stru);
//				}
//				nwAddObjectsStruct stru;
//				for (auto& pair : objects)
//				{
//					auto& item = stru.items.emplace_back();
//					item.prefab_id = pair.second->prefab_id;
//					item.id = pair.second->uid;
//				}
//				pack_msg(res, nwAddObjects, stru);
//				so_server->send(so_id, res.str());
//			}
//		}
//		{	// info new and removed objects
//			std::ostringstream res;
//			if (!new_objects.empty())
//			{
//				nwAddObjectsStruct stru;
//				for (auto& o : new_objects)
//				{
//					auto& item = stru.items.emplace_back();
//					item.prefab_id = o.first;
//					item.id = o.second;
//				}
//				pack_msg(res, nwAddObjects, stru);
//				new_objects.clear();
//			}
//			if (!removed_objects.empty())
//			{
//				nwRemoveObjectsStruct stru;
//				for (auto id : removed_objects)
//					stru.ids.push_back(id);
//				pack_msg(res, nwRemoveObjects, stru);
//				removed_objects.clear();
//			}
//			if (auto str = res.str(); !str.empty())
//			{
//				for (auto& f : nw_players)
//				{
//					for (auto so_id : f.second)
//					{
//						// new players has been infoed above
//						if (std::find(nw_new_players.begin(), nw_new_players.end(), so_id) == nw_new_players.end())
//							so_server->send(so_id, str);
//					}
//				}
//			}
//		}
//		for (auto& f : nw_players)
//		{
//			std::ostringstream res;
//			nwUpdateObjectsStruct stru_update;
//			for (auto& pair : objects)
//			{
//				auto entity = pair.second->entity;
//				auto harvester = entity->get_component_t<cNWDataHarvester>();
//				if (!harvester) continue;
//				auto has_vision = get_vision(f.first, entity->node()->pos);
//				nwUpdateObjectsStruct::Item item;
//				item.obj_id = pair.first;
//				for (auto i = 0; i < harvester->targets.size(); i++)
//				{
//					nwUpdateObjectsStruct::Comp citem;
//					citem.idx = i;
//
//					auto comp = entity->components[i].get();
//					auto ui = find_udt(comp->type_hash);
//					for (auto& pair : harvester->targets[i])
//					{
//						if (!has_vision && pair.first != "visible_flags"_h)
//							continue;
//						if (pair.second.first & f.first)
//						{
//							auto vi = ui->find_variable(pair.first); auto len = vi->type->size;
//							citem.names.push_back(pair.first);
//							auto sz = (uint)citem.datas.size();
//							citem.datas.resize(sz + len);
//							memcpy(citem.datas.data() + sz, (char*)comp + vi->offset, len);
//							pair.second.first &= ~f.first;
//						}
//					}
//					if (!citem.names.empty())
//						item.comps.push_back(std::move(citem));
//				}
//				if (!item.comps.empty())
//					stru_update.items.push_back(std::move(item));
//			}
//			if (!stru_update.items.empty())
//				pack_msg(res, nwUpdateObjects, stru_update);
//			if (auto str = res.str(); !str.empty())
//			{
//				for (auto so_id : f.second)
//					so_server->send(so_id, str);
//			}
//		}
//		nw_new_players.clear();
//	}
//}
