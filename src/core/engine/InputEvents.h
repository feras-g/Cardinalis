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
	LSHIFT = 1 << 10,
	T = 1 << 11,
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
		case Key::T: { return 'T'; }
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
	// Mouse click
	int curr_click_px = 0;
	int curr_click_py = 0;

	int last_click_px = 0;
	int last_click_py = 0;

	float wheel_zdelta = 0.0f;
	// Mouse movement
	int curr_pos_x = 0;
	int curr_pos_y = 0;

	// Mouse events
	bool b_first_click = false;
	bool b_lmb_click  = false;
	bool b_rmb_click = false;
	bool b_mmb_click  = false;
	bool b_wheel_scroll = false;
};

struct KeyEvent
{
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


