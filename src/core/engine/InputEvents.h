#pragma once

#include <string>

// States
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

static std::string keyToString(Key key)
{
	std::string out;

	switch (key)
	{
		case Key::Z: { out += 'Z'; } break;
		case Key::S: { out += 'S'; } break;
		case Key::Q: { out += 'Q'; } break;
		case Key::D: { out += 'D'; } break;
		case Key::R: { out += 'R'; } break;
		case Key::UP: { out += "ARROW_UP"; } break;
		case Key::DOWN: { out += "ARROW_DOWN"; } break;
		case Key::LEFT: { out += "ARROW_LEFT"; } break;
		case Key::RIGHT: { out += "ARROW_RIGHT"; } break;
		case Key::LSHIFT: { out += "LEFT_SHIFT"; } break;
		default: out += "Unknown"; 
	}

	return out;
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


inline Key operator|(Key a, Key b)
{
	return static_cast<Key>(static_cast<int>(a) | static_cast<int>(b));
}

struct MouseEvent
{
	int px, py;
	int lastClickPosX;
	int lastClickPosY;

	bool bFirstClick = false;
	bool bLeftClick = false;
};

struct KeyEvent
{
	Key key = Key::NONE;
	bool pressed = false;
};
