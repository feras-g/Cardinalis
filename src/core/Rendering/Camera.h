#pragma once

#include "glm/gtc/quaternion.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "../engine/InputEvents.h"

class Application;

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
	CameraController()
	: m_position( { 0, 0, 5 }), m_forward( { 0, 0, 0 }), m_up( { 0, 1, 0 }), yaw(0.0f), pitch(0.0f), m_last_mouse_pos(0.0f)
	{
		update_view();
	}
	CameraController(glm::vec3 pos, glm::vec3 rot, glm::vec3 target, glm::vec3 up)
	: m_position(pos), yaw(0.0f), pitch(0.0f), m_forward(target), m_up(up), m_last_mouse_pos(0.0f)
	{
		update_view();
	}

	void translate(float dt, KeyEvent event);
	void rotate(float dt, MouseEvent event);

	glm::mat4& update_view();

	float mouse_speed = 1.0f;

	Movement m_movement = Movement::NONE;
	glm::mat4 m_view;
	glm::vec3 m_position;
	glm::vec3 m_forward = { 0, 0, -1 };
	glm::vec3 m_up = { 0, 1, 0 };
	glm::vec3 m_right = { 1, 0, 0 };
	glm::vec2 m_last_mouse_pos;


	float move_speed = 1.f;
	bool b_first_click = true;

	float yaw = 0.0f;
	float pitch = 0.0f;
};

constexpr float ASPECT_16_9 = 16.0F / 9;

class Camera
{
public:
	Camera();
	Camera(CameraController controller, float fov, float aspect_ratio, float n, float f);

	const glm::mat4& get_proj() const;
	const glm::mat4& get_view() const;

	void update_aspect_ratio(float aspect);
	void update_proj();

	void translate(float dt, KeyEvent event);
	void rotate(float dt, MouseEvent event);

	CameraController controller;

	float fov;
	float aspect_ratio;

	float znear;
	float zfar;

	enum class ProjectionMode
	{
		NONE = 0,
		PERSPECTIVE  = 1,
		ORTHOGRAPHIC = 2
	} projection_mode;

	glm::mat4 projection;

	/* Orthographic */
	float left = -1, right = 1, bottom = -1, top = 1;

protected:
};
