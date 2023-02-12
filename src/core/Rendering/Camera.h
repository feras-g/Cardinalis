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
		: m_position(pos), m_forward(target), m_up(up), m_last_mouse_pos(0.0f), m_velocity(0.0f) 
	{
		GetView();
	}

	void UpdateTranslation(float dt);
	void UpdateRotation(float dt, const glm::vec2& mouse_pos);
	glm::mat4 GetView();
	struct
	{
		float mouse_speed	= 0.1f;
		float accel_factor	= 15.0f;
		float damping		= 0.25f;
		float max_velocity  = 15.0f;
	} params;

	Movement m_movement = Movement::NONE;
	glm::mat4 m_view;
	glm::vec3 m_position;
	glm::vec3 m_forward = { 0, 0, -1 };
	glm::vec3 m_up    =   {0, 1, 0 };
	glm::vec3 m_right =   {1, 0, 0};
	glm::vec3 m_velocity;
	glm::vec2 m_last_mouse_pos;


	bool m_rotate = false;
	float m_pitch = 0.0f;
	float m_yaw   = 0.0f;

private:
};

class Camera
{

};

