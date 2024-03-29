#pragma once

#include "../head.h"

// Reflect ctor
struct cButton : Component
{
	// Reflect requires
	cStretchedImagePtr image_comp;
	// Reflect requires
	cReceiverPtr receiver;

	// Reflect
	std::filesystem::path normal_image_name;
	// Reflect
	void set_normal_image_name(const std::filesystem::path& image_name);
	// Reflect
	std::filesystem::path hovered_image_name;
	// Reflect
	void set_hovered_image_name(const std::filesystem::path& image_name);
	// Reflect
	std::filesystem::path pressed_image_name;
	// Reflect
	void set_pressed_image_name(const std::filesystem::path& image_name);

	// Reflect
	std::filesystem::path click_sound_name;
	// Reflect
	void set_click_sound_name(const std::filesystem::path& sound_name);

	~cButton();
	void update_state();
	void on_init() override;

	graphics::ImagePtr normal_image = nullptr;
	graphics::ImagePtr hovered_image = nullptr;
	graphics::ImagePtr pressed_image = nullptr;
	audio::BufferPtr click_sound = nullptr;

	struct Create
	{
		virtual cButtonPtr operator()(EntityPtr) = 0;
	};
	// Reflect static
	EXPORT static Create& create;
};
