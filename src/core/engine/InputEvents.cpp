#include "InputEvents.h"

#include "core/engine/Window.h"

bool KeyEvent::is_key_pressed_async(Key key) const
{
	return h_window->AsyncKeyState(key);
}

std::string KeyEvent::to_string()
{
	std::string out;

	if (key == (Key::Z)) { out += "Z "; };
	if (key == (Key::S)) { out += "S "; };
	if (key == (Key::Q)) { out += "Q "; };
	if (key == (Key::D)) { out += "D "; };
	if (key == (Key::R)) { out += "R "; };
	if (key == (Key::UP)) { out += "ARROW_UP "; };
	if (key == (Key::DOWN)) { out += "ARROW_DOWN "; };
	if (key == (Key::LEFT)) { out += "ARROW_LEFT "; };
	if (key == (Key::RIGHT)) { out += "ARROW_RIGHT "; };
	if (key == (Key::LSHIFT)) { out += "L_SHIFT "; };

	return out;
};