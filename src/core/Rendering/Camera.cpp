#include "Camera.h"

#include "imgui.h"

camera::camera()
	: camera(glm::radians(50.0f), 1.0f, 0.25f, 250.0f)
{

}
camera::camera(float fov, float aspect_ratio, float znear, float zfar)
{
	init_projection(fov, aspect_ratio, znear, zfar);
	init_view(glm::vec3(0,0,0), axes.forward, axes.up);
}

void camera::init_view(glm::vec3 eye, glm::vec3 center, glm::vec3 up)
{
	position = eye;
	view = glm::lookAt(position, position + center, up);
}

void camera::init_projection(float fov, float aspect_ratio, float znear, float zfar)
{
	this->fov = fov;
	this->aspect_ratio = aspect_ratio;
	this->znear = znear;
	this->zfar = zfar;
	projection = glm::perspectiveRH_ZO(fov, aspect_ratio, znear, zfar);
}

void camera::update_view()
{
	//forward = glm::normalize(forward);
	//right = glm::normalize(glm::cross(forward, { 0,1,0 }));
	//up = glm::normalize(glm::cross(right, forward));

	view = glm::lookAtRH(position, position + forward, up);
}

void camera::update_projection()
{
	projection = glm::perspectiveRH_ZO(fov, aspect_ratio, znear, zfar);
}

void camera::update_aspect_ratio(float aspect_ratio)
{
	this->aspect_ratio = aspect_ratio;
	update_projection();
}

void camera::translate(glm::vec3 offset)
{
	position += offset * move_speed;
	update_view();
}

void camera::rotate(glm::vec2 mouse_pos)
{
	if (first_mouse_click)
	{
		last_click_pos = mouse_pos;
		first_mouse_click = false;
		return;
	}

	float xoffset = mouse_pos.x - last_click_pos.x;
	float yoffset = last_click_pos.y - mouse_pos.y;
	last_click_pos.x = mouse_pos.x;
	last_click_pos.y = mouse_pos.y;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	glm::vec2 rotation(pitch, yaw);

	pitch = std::clamp(pitch, -89.0f, 89.0f);

	forward.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
	forward.y = sin(glm::radians(rotation.x));
	forward.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));

	forward = glm::normalize(forward);
}

void camera::show_ui()
{
	if (ImGui::Begin("Camera"))
	{
		if (ImGui::InputFloat3("Position", glm::value_ptr(position)) ||
			ImGui::InputFloat3("Forward", glm::value_ptr(forward)) ||
			ImGui::InputFloat3("Up", glm::value_ptr(up)) ||
			ImGui::InputFloat3("Right", glm::value_ptr(right)))
		{
			update_view();
		}

		ImGui::SeparatorText("Controls");
		ImGui::InputFloat3("Move Speed", glm::value_ptr(move_speed));

		ImGui::Text("View Matrix");
		ImGui::InputFloat4("R0 - View", glm::value_ptr(view[0]));
		ImGui::InputFloat4("R1 - View", glm::value_ptr(view[1]));
		ImGui::InputFloat4("R2 - View", glm::value_ptr(view[2]));
		ImGui::InputFloat4("R3 - View", glm::value_ptr(view[3]));


		ImGui::Text("Projection Matrix");
		ImGui::InputFloat4("R0 - Projection", glm::value_ptr(projection[0]));
		ImGui::InputFloat4("R1 - Projection", glm::value_ptr(projection[1]));
		ImGui::InputFloat4("R2 - Projection", glm::value_ptr(projection[2]));
		ImGui::InputFloat4("R3 - Projection", glm::value_ptr(projection[3]));

		if (ImGui::InputFloat("Z Near", &znear) || ImGui::InputFloat("Z Far", &zfar))
		{
			update_projection();
		}
	}
	ImGui::End();
}
