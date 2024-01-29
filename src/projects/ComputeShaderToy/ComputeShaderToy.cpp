#include "ComputeShaderToy.h"

#include "engine/Window.h"
#include "rendering/vulkan/VulkanDebugUtils.h"
#include "rendering/vulkan/VulkanRenderInterface.h"

ComputeShaderToy::ComputeShaderToy(const char* title, uint32_t width, uint32_t height)
	: Application(title, width, height)
{
	
}

void ComputeShaderToy::init()
{

}

void ComputeShaderToy::compose_gui()
{
	m_gui.begin();
	m_gui.show_demo();
	m_gui.end();
}

void ComputeShaderToy::update_frame_ubo()
{

}

void ComputeShaderToy::update(float t, float dt)
{
	compose_gui();
}

void ComputeShaderToy::render()
{
	VkCommandBuffer cmd_buffer = ctx.get_current_frame().cmd_buffer;
	ctx.swapchain->clear_color(cmd_buffer);
	m_gui.render(cmd_buffer);
}

void ComputeShaderToy::update_gpu_buffers()
{
}

void ComputeShaderToy::create_scene()
{

}

void ComputeShaderToy::update_instances_ssbo()
{

}

void ComputeShaderToy::exit()
{
	m_gui.exit();
}

void ComputeShaderToy::on_window_resize()
{
	Application::on_window_resize();
	m_gui.on_window_resize();
}

void ComputeShaderToy::on_mouse_move(MouseEvent event)
{
	Application::on_mouse_move(event);
}

void ComputeShaderToy::on_key_event(KeyEvent event)
{
	Application::on_key_event(event);
}

void ComputeShaderToy::on_mouse_up(MouseEvent event)
{
	Application::on_mouse_up(event);
}

void ComputeShaderToy::on_mouse_down(MouseEvent event)
{
	Application::on_mouse_down(event);
}