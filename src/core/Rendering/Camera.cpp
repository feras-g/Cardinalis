#include "Camera.h"

#include "core/engine/EngineLogger.h"
#include "core/engine/Application.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec3.hpp>

void CameraController::translate(float dt, KeyEvent event)
{
	if (event.is_key_pressed_async(Key::Z))      m_movement = m_movement | Movement::FORWARD;
	if (event.is_key_pressed_async(Key::Q))      m_movement = m_movement | Movement::LEFT;
	if (event.is_key_pressed_async(Key::S))      m_movement = m_movement | Movement::BACKWARD;
	if (event.is_key_pressed_async(Key::D))      m_movement = m_movement | Movement::RIGHT;
	if (event.is_key_pressed_async(Key::SPACE))  m_movement = m_movement | Movement::UP;
	if (event.is_key_pressed_async(Key::LSHIFT)) m_movement = m_movement | Movement::DOWN;

	glm::vec3 direction(0.0f);

	if ((m_movement & Movement::UP)   == Movement::UP)			  direction += m_up * move_speed;
	if ((m_movement & Movement::DOWN) == Movement::DOWN)		  direction -= m_up * move_speed;

	if ((m_movement & Movement::RIGHT) == Movement::RIGHT)		  direction += m_right * move_speed;
	if ((m_movement & Movement::LEFT)  == Movement::LEFT)		  direction -= m_right * move_speed;

	if ((m_movement & Movement::FORWARD)  == Movement::FORWARD)   direction += m_forward * move_speed;
	if ((m_movement & Movement::BACKWARD) == Movement::BACKWARD)  direction -= m_forward * move_speed;

	m_position += direction ;

	m_movement = Movement::NONE;

	update_view();
}

void CameraController::rotate(float dt, MouseEvent event)
{
	if (b_first_click)
	{
		m_last_mouse_pos.x = (float)event.px;
		m_last_mouse_pos.y = (float)event.py;
		b_first_click = false;
	}

	float xoffset = (float)event.px - m_last_mouse_pos.x;
	float yoffset = m_last_mouse_pos.y - (float)event.py;
	m_last_mouse_pos.x = (float)event.px;
	m_last_mouse_pos.y = (float)event.py;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw   += xoffset;
	pitch += yoffset;

	pitch = std::clamp(pitch, -89.0f, 89.0f);

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	m_forward = glm::normalize(direction);
	update_view();
}

glm::mat4& CameraController::update_view()
{
	m_right   = glm::normalize(glm::cross(m_forward, { 0,1,0 }));
	m_up      = glm::normalize(glm::cross(m_right, m_forward));
	m_forward = glm::cross(m_up, m_right);

	m_view = glm::lookAt(m_position, m_position + m_forward, m_up);

	return m_view;
}

Camera::Camera()
: controller({ 0,1.0f,0 }, { 0,-180,0 }, { 0,0,1 }, { 0,1,0 }), fov(45.0f), aspect_ratio(ASPECT_16_9), znear(0.25f), zfar(250.0f), projection_mode(ProjectionMode::PERSPECTIVE)
{
	update_proj();
}

Camera::Camera(CameraController controller, float fov, float aspect_ratio, float zNear, float zFar)
	: controller(controller), fov(fov), aspect_ratio(ASPECT_16_9), znear(zNear), zfar(zFar), projection_mode(ProjectionMode::PERSPECTIVE)
{
	update_proj();
}


void Camera::update_proj()
{
	if (projection_mode == ProjectionMode::PERSPECTIVE)
	{
		projection = glm::perspective(glm::radians(fov), aspect_ratio, znear, zfar);
	}
	else if (projection_mode == ProjectionMode::ORTHOGRAPHIC)
	{
		projection = glm::ortho(left, right, bottom, top, znear, zfar);
	}
}

void Camera::translate(float dt, KeyEvent event)
{
	controller.translate(dt, event);
}

void Camera::rotate(float dt, MouseEvent event)
{
	controller.rotate(dt, event);
}

void Camera::update_aspect_ratio(float aspect)
{
	aspect_ratio = aspect;
	update_proj();
}

const glm::mat4& Camera::get_proj() const
{
	return projection;
}

const glm::mat4& Camera::get_view() const
{
	return controller.m_view;
}