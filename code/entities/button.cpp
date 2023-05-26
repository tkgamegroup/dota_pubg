#include <flame/audio/buffer.h>
#include <flame/graphics/image.h>
#include <flame/universe/components/stretched_image.h>
#include <flame/universe/components/receiver.h>
#include <flame/universe/systems/input.h>

#include "button.h"
#include "../audio.h"

void cButton::set_normal_image_name(const std::filesystem::path& image_name)
{
	if (normal_image_name == image_name)
		return;

	auto old_one = normal_image;
	if (!normal_image_name.empty())
		AssetManagemant::release(Path::get(normal_image_name));

	normal_image_name = image_name;
	normal_image = nullptr;
	if (!normal_image_name.empty())
	{
		AssetManagemant::get(Path::get(normal_image_name));
		normal_image = !normal_image_name.empty() ? graphics::Image::get(normal_image_name, false) : nullptr;
	}

	if (!normal_image_name.empty())
		AssetManagemant::release(Path::get(normal_image_name));
	normal_image_name = image_name;
	if (!normal_image_name.empty())
		AssetManagemant::get(Path::get(normal_image_name));

	update_state();

	if (old_one)
		graphics::Image::release(old_one);
	data_changed("normal_image_name"_h);
}

void cButton::set_hovered_image_name(const std::filesystem::path& image_name)
{
	if (hovered_image_name == image_name)
		return;

	auto old_one = hovered_image;
	if (!hovered_image_name.empty())
		AssetManagemant::release(Path::get(hovered_image_name));

	hovered_image_name = image_name;
	hovered_image = nullptr;
	if (!hovered_image_name.empty())
	{
		AssetManagemant::get(Path::get(hovered_image_name));
		hovered_image = !hovered_image_name.empty() ? graphics::Image::get(hovered_image_name, false) : nullptr;
	}

	if (!hovered_image_name.empty())
		AssetManagemant::release(Path::get(hovered_image_name));
	hovered_image_name = image_name;
	if (!hovered_image_name.empty())
		AssetManagemant::get(Path::get(hovered_image_name));

	update_state();

	if (old_one)
		graphics::Image::release(old_one);
	data_changed("hovered_image_name"_h);
}

void cButton::set_pressed_image_name(const std::filesystem::path& image_name)
{
	if (pressed_image_name == image_name)
		return;

	auto old_one = pressed_image;
	if (!pressed_image_name.empty())
		AssetManagemant::release(Path::get(pressed_image_name));

	pressed_image_name = image_name;
	pressed_image = nullptr;
	if (!pressed_image_name.empty())
	{
		AssetManagemant::get(Path::get(pressed_image_name));
		pressed_image = !pressed_image_name.empty() ? graphics::Image::get(pressed_image_name, false) : nullptr;
	}

	if (!pressed_image_name.empty())
		AssetManagemant::release(Path::get(pressed_image_name));
	pressed_image_name = image_name;
	if (!pressed_image_name.empty())
		AssetManagemant::get(Path::get(pressed_image_name));

	update_state();

	if (old_one)
		graphics::Image::release(old_one);
	data_changed("pressed_image_name"_h);
}

void cButton::set_click_sound_name(const std::filesystem::path& sound_name)
{
	if (click_sound_name == sound_name)
		return;

	auto old_one = click_sound;
	if (!click_sound_name.empty())
		AssetManagemant::release(Path::get(click_sound_name));

	click_sound_name = sound_name;
	click_sound = nullptr;
	if (!click_sound_name.empty())
	{
		AssetManagemant::get(Path::get(click_sound_name));
		click_sound = !click_sound_name.empty() ? audio::Buffer::get(click_sound_name) : nullptr;
	}

	if (!click_sound_name.empty())
		AssetManagemant::release(Path::get(click_sound_name));
	click_sound_name = sound_name;
	if (!click_sound_name.empty())
		AssetManagemant::get(Path::get(click_sound_name));

	if (old_one)
		audio::Buffer::release(old_one);
	data_changed("click_sound_name"_h);
}

void cButton::update_state()
{
	auto input = sInput::instance();
	if (input->active_receiver == receiver)
		image_comp->set_image_name(pressed_image ? L"0x" + wstr_hex((uint64)pressed_image) : L"");
	else if (input->hovering_receiver == receiver)
		image_comp->set_image_name(hovered_image ? L"0x" + wstr_hex((uint64)hovered_image) : L"");
	else
		image_comp->set_image_name(normal_image ? L"0x" + wstr_hex((uint64)normal_image) : L"");
}

void cButton::on_init()
{
	receiver->event_listeners.add([this](uint type, const vec2&) {
		switch (type)
		{
		case "mouse_entered"_h:
		case "mouse_left"_h:
		case "gain_focus"_h:
		case "lost_focus"_h:
			update_state();
			break;
		case "mouse_down"_h:
			if (click_sound)
				play_global_sound(click_sound, 1.f);
			break;
		}
	});
}

struct cButtonCreate : cButton::Create
{
	cButtonPtr operator()(EntityPtr e) override
	{
		if (e == INVALID_POINTER)
			return nullptr;
		return new cButton;
	}
}cButton_create;
cButton::Create& cButton::create = cButton_create;
