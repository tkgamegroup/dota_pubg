#pragma once

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

struct ItemInstance;
struct ItemShortcut : Shortcut
{
	ItemInstance* ins;

	ItemShortcut(ItemInstance* ins);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

struct AbilityInstance;
struct AbilityShortcut : Shortcut
{
	AbilityInstance* ins;

	AbilityShortcut(AbilityInstance* ins);

	void draw(ImDrawList* dl, const vec2& p0, const vec2& p1) override;

	void click() override;
};

extern std::unique_ptr<Shortcut> shortcuts[10];
