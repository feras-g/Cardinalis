#include "Camera.h"

#include "Core/EngineLogger.h"

#include <glm/gtx/rotate_vector.hpp>

void CameraController::UpdateRotation(const glm::vec2& mouse_pos)
{
	// Mouse delta in range [-1;1]
	m_rotation.y += (m_last_mouse_pos.x - mouse_pos.x) * params.mouse_speed;
	m_rotation.x += (m_last_mouse_pos.y - mouse_pos.y) * params.mouse_speed;

	if (m_rotation.x > 89.9f)  m_rotation.x = 89.9f;
	if (m_rotation.y < -89.9f) m_rotation.y = -89.9f;

	m_last_mouse_pos = mouse_pos;
}

void CameraController::UpdateTranslation(float dt)
{
	// Translation
	glm::vec3 acceleration(0.0f);
	if ((m_movement & Movement::UP) == Movement::UP)			acceleration += m_up;
	if ((m_movement & Movement::DOWN) == Movement::DOWN)		acceleration -= m_up;

	if ((m_movement & Movement::RIGHT) == Movement::RIGHT)		acceleration += m_right;
	if ((m_movement & Movement::LEFT) == Movement::LEFT)		acceleration -= m_right;

	if ((m_movement & Movement::FORWARD) == Movement::FORWARD)   acceleration += m_forward;
	if ((m_movement & Movement::BACKWARD) == Movement::BACKWARD) acceleration -= m_forward;

	if (glm::length(acceleration) == 0.0f)
	{
		// Decelerate progressively
		float damping_factor = (1.0f / params.damping) * dt;
		m_velocity -= m_velocity * damping_factor;
	}
	else
	{
		// Integrate velocity
		m_velocity += params.accel_factor * acceleration * static_cast<float>(dt);
		if (glm::length(m_velocity) > params.max_velocity)
		{
			m_velocity = glm::normalize(m_velocity) * params.max_velocity;
		}
	}

	//LOG_INFO("Speed: {} / Accel: {}", glm::length(m_velocity), glm::length(acceleration));

	m_position = m_position + m_velocity * dt;

	m_movement = Movement::NONE;
}

glm::mat4& CameraController::GetView()
{
	m_forward.x = cos(glm::radians(m_rotation.x)) * sin(glm::radians(m_rotation.y));
	m_forward.y = sin(glm::radians(m_rotation.x));
	m_forward.z = cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));

	m_forward = glm::normalize(m_forward);
	m_right   = glm::normalize(glm::cross(m_forward, { 0,1,0 }));
	m_up      = glm::normalize(glm::cross(m_right, m_forward));
	
	m_view = glm::lookAt(m_position, m_position+m_forward, m_up);

	return m_view;
}

Camera::Camera() 
	: controller({ 0, 0, -5 }, { 0, 0, 1 }, { 0, 1, 0 }), fov(45.0f), aspect_ratio(ASPECT_16_9), near_far(0.0f, 1000.0f)
{
	UpdateProjection();
}

Camera::Camera(CameraController controller, float fov, float aspect_ratio, float zNear, float zFar)
	: controller(controller), fov(fov), aspect_ratio(ASPECT_16_9), near_far(zNear, zFar)
{
	UpdateProjection();
}


void Camera::UpdateProjection()
{
	projection = glm::perspective(fov, aspect_ratio, near_far.x, near_far.y);
}

void Camera::UpdateAspectRatio(float aspect)
{
	projection = glm::perspective(fov, aspect, near_far.x, near_far.y);
}

glm::mat4& Camera::GetProj()
{
	return projection;
}

glm::mat4& Camera::GetView()
{
	return controller.GetView();
}