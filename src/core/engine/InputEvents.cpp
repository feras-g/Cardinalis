#include "InputEvents.h"

#include "core/engine/Window.h"

bool KeyEvent::is_key_pressed_async(Key key) const
{
	return h_window->AsyncKeyState(key);
}

std::string KeyEvent::to_string()
{
	std::string out;

	if (contains(Key::Z)) { out += "Z "; };
	if (contains(Key::S)) { out += "S "; };
	if (contains(Key::Q)) { out += "Q "; };
	if (contains(Key::D)) { out += "D "; };
	if (contains(Key::R)) { out += "R "; };
	if (contains(Key::UP)) { out += "ARROW_UP "; };
	if (contains(Key::DOWN)) { out += "ARROW_DOWN "; };
	if (contains(Key::LEFT)) { out += "ARROW_LEFT "; };
	if (contains(Key::RIGHT)) { out += "ARROW_RIGHT "; };
	if (contains(Key::LSHIFT)) { out += "L_SHIFT "; };

	return out;
};