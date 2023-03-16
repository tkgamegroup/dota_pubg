#pragma once

#include "../head.h"

struct ImDrawList;
struct Shortcut
{
	enum Type
	{
		tNull,
		tItem,
		tAbility
	};

	Type type = tNull;
	int id = -1;
	KeyboardKey key = KeyboardKey_Count;

	virtual void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) {}
	virtual void click() {}
};

struct ItemShortcut : Shortcut
{
	cItemPtr item;

	ItemShortcut(cItemPtr item);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

struct AbilityShortcut : Shortcut
{
	cAbilityPtr ability;

	AbilityShortcut(cAbilityPtr ability);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

extern std::unique_ptr<Shortcut> shortcuts[10];

void init_shortcuts();
