#pragma once

#include "core/engine/common.h"

class camera
{
public:
	camera();
	camera(float fov, float aspect_ratio, float znear, float zfar);

	void init_view(glm::vec3 eye, glm::vec3 center, glm::vec3 up);
	void init_projection(float fov, float aspect_ratio, float znear, float zfar);

	void update_view();
	void update_projection();
	void update_aspect_ratio(float aspect_ratio);
	void translate(glm::vec3 offset);
	void rotate(glm::vec2 delta);
	void show_ui();

	glm::vec3 move_speed = { 2, 2, 2 };

	glm::mat4x4 projection;
	glm::mat4x4 view;

	glm::vec3 position{ 0, 0, 0 };
	glm::vec3 forward{ 0, 0, 1 };
	glm::vec3 up{ 0, 1, 0 };
	glm::vec3 right{ 1, 0, 0 };

	struct 
	{
		glm::vec3 up{ 0, 1, 0 };
		glm::vec3 right{ 1, 0, 0 };
		glm::vec3 forward{ 0, 0, 1 };
	} axes;

	float aspect_ratio;
	float fov;
	float znear;
	float zfar;


	float yaw = 0.0f;
	float pitch = 0.0f;

	bool first_mouse_click = false;
	glm::vec2 last_click_pos;
};


