#include "floating_text.h"

static std::vector<FloatingText> floating_texts;

void add_floating_text(const vec3& pos, const std::string& text, const cvec4& color)
{
	auto& t = floating_texts.emplace_back();
	t.ticks = 30;
	t.pos = pos;
	t.text = text;
	t.color = color;
}
