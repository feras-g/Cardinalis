#include "Camera.h"

#include "Core/EngineLogger.h"

void CameraController::UpdateRotation(double dt, const glm::vec2& mouse_pos)
{
	//// Rotation
	//const glm::vec2 delta_mouse_pos = mouse_pos - m_last_mouse_pos;

	//const glm::quat delta_quat = glm::quat(glm::vec3(params.mouse_speed * delta_mouse_pos.y, params.mouse_speed * delta_mouse_pos.x, 0.0f));
	//m_orientation = glm::normalize(delta_quat * m_orientation);
	//m_last_mouse_pos = mouse_pos;
}

void CameraController::UpdateTranslation(double dt)
{
	// Translation
	glm::vec3 right = glm::cross(m_forward, m_up);

	glm::vec3 acceleration(0.0f);
	if ((m_movement & Movement::UP) == Movement::UP)			acceleration += m_up;
	if ((m_movement & Movement::DOWN) == Movement::DOWN)		acceleration -= m_up;

	if ((m_movement & Movement::RIGHT) == Movement::RIGHT)		acceleration += right;
	if ((m_movement & Movement::LEFT) == Movement::LEFT)		acceleration -= right;

	if ((m_movement & Movement::FORWARD) == Movement::FORWARD)   acceleration += m_forward;
	if ((m_movement & Movement::BACKWARD) == Movement::BACKWARD) acceleration -= m_forward;

	if (acceleration.length() == 0.0f)
	{
		// Decelerate progressively
		float damping_factor = (1.0f / params.damping) * dt;
		m_speed -= m_speed * std::min(damping_factor, 1.0F);
	}
	else
	{
		// Euler integration
		m_speed += params.accel_factor * acceleration * static_cast<float>(dt);
		if (glm::length(m_speed) > params.max_speed)
		{
			m_speed = glm::normalize(m_speed) * params.max_speed;
		}
	}

	m_position += static_cast<float>(dt) * m_speed;// m_speed;

	m_movement = Movement::NONE;
	m_speed = glm::vec3(0.0f);

	LOG_INFO("Speed : ({0}, {1}, {2}) Position : ({3}, {4}, {5})", params.accel_factor, m_speed.y, m_speed.z, m_position.x, m_position.y, m_position.z);
}

glm::mat4 CameraController::GetView()
{
	m_up	  = glm::normalize(m_up);
	m_forward = glm::normalize(m_forward);

	return glm::lookAt(m_position, m_position + m_forward, m_up);
}
