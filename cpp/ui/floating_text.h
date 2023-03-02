#pragma once

#include "../head.h"

struct FloatingText
{
	uint ticks;
	vec3 pos;
	std::string text;
	vec2 size = vec2(0.f);
	cvec4 color;
};

void add_floating_text(const vec3& pos, const std::string& text, const cvec4& color);
