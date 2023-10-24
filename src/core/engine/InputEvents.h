#pragma once

#include <string>
#include <glm/vec2.hpp>

enum class Key
{
	NONE = 0,
	Z = 1 << 0,
	Q = 1 << 1,
	S = 1 << 2,
	D = 1 << 3,
	R = 1 << 4,
	UP = 1 << 5,
	DOWN = 1 << 6,
	LEFT = 1 << 7,
	RIGHT = 1 << 8,
	SPACE = 1 << 9,
	LSHIFT = 1 << 10
};

static int keyToWindows(Key key)
{
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	switch (key)
	{
		case Key::Z: { return 'Z'; }
		case Key::S: { return 'S'; }
		case Key::Q: { return 'Q'; }
		case Key::D: { return 'D'; }
		case Key::R: { return 'R'; }
		case Key::UP:	{ return 0x25; }
		case Key::DOWN: { return 0x28; }
		case Key::LEFT: { return 0x25; }
		case Key::RIGHT: { return 0x27; }
		case Key::SPACE: { return 0x20; }
		case Key::LSHIFT: { return 0x10; }
	}

	return 0;
}


static Key operator|(Key a, Key b)
{
	return static_cast<Key>(static_cast<int>(a) | static_cast<int>(b));
}

static Key operator&(Key a, Key b)
{
	return static_cast<Key>(static_cast<int>(a) & static_cast<int>(b));
}

static bool operator==(Key a, Key b)
{
	return static_cast<int>(a) == static_cast<int>(b);
}

static Key operator~(Key a)
{
	return static_cast<Key>(static_cast<int>(a));
}

class Window;

struct MouseEvent
{
	int px, py;
	int last_click_px;
	int last_click_py;

	bool b_first_lmb_click = false;
	bool b_lmb_click  = false;
};

struct KeyEvent
{
	void append(Key other) { key = key | other; };
	void remove(Key other) { key = key & ~other; };
	bool contains(Key other) const { return (key & other) == other; };
	bool is_key_pressed_async(Key key) const;
	std::string to_string();

	Key key = Key::NONE;
	bool pressed = false;

	Window const* h_window;
};


class EventManager
{
public:
	EventManager(Window const* window_handle) : h_window(window_handle) { key_event.h_window = window_handle; }

	MouseEvent mouse_event;
	KeyEvent key_event;

protected:
	Window const* h_window;
};


