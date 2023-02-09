#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"

enum class Movement
{
	NONE = 0,
	FORWARD = 1 << 0,
	BACKWARD = 1 << 1,
	LEFT = 1 << 2,
	RIGHT = 1 << 3,
	UP = 1 << 4,
	DOWN = 1 << 5
};

static Movement operator|(Movement a, Movement b)
{
	return static_cast<Movement>(static_cast<int>(a) | static_cast<int>(b));
}

static Movement operator&(Movement a, Movement b)
{
	return static_cast<Movement>(static_cast<int>(a) & static_cast<int>(b));
}

static bool operator==(Movement a, Movement b)
{
	return static_cast<int>(a) == static_cast<int>(b);
}

class CameraController
{
public:

	CameraController() = default;
	CameraController(glm::vec3 pos, glm::vec3 target, glm::vec3 up)
		: m_position(pos), m_forward(target), m_up(up), m_last_mouse_pos(0.0f), m_speed(0.0f) {}

	void UpdateRotation(double dt, const glm::vec2& mouse_pos);
	void UpdateTranslation(double dt);
	glm::mat4 GetView();
	struct
	{
		float mouse_speed	= 5.0f;
		float accel_factor	= 150.0f;
		float damping		= 0.2f;
		float max_speed		= 10.0f;
	} params;

	Movement m_movement = Movement::NONE;
	glm::vec3 m_position;
	glm::vec3 m_forward;
	glm::vec3 m_up;
	glm::vec3 m_speed;
	glm::vec2 m_last_mouse_pos;

private:
};

class Camera
{

};

