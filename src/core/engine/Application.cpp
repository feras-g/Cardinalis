#include "Application.h"

#include <iostream>

#include "core/engine/logger.h"
#include "core/engine/Window.h"
#include "core/rendering/FrameCounter.h"
#include "core/rendering/vulkan/Renderers/VulkanImGuiRenderer.h"
#include "core/rendering/vulkan/vk_frame.hpp"
#include "core/rendering/vulkan/VulkanRenderInterface.h"
#include "core/rendering/vulkan/VulkanRendererBase.h"
#include "core/engine/vulkan/objects/vk_debug_marker.hpp"

Application::Application(const char* title, uint32_t width, uint32_t height)
    : b_init_success(false), m_debug_name(title)
{
    m_window.reset(
        new Window({.width = width, .height = height, .title = title}, this));

    m_event_manager.reset(new EventManager(m_window.get()));

    logger::init("Engine");

    m_rhi.reset(new RenderInterface(title, 1, 3, 0));
    m_rhi->init();
#ifdef _WIN32
    m_rhi->create_surface(m_window.get());
    m_rhi->create_swapchain();
#else
    assert(false);
#endif  // _WIN32

    b_init_success = true;
}

Application::~Application()
{
    m_rhi->terminate();
    m_rhi.release();
    m_window.release();
}

void Application::run()
{
    init();

    while (!m_window->is_closed())
    {
        m_time = (float)m_window->GetTime();
        Timestep timestep = m_time - m_last_frametime;
        m_last_frametime = m_time;
        m_delta_time = timestep.GetSeconds();
        m_window->handle_events();
        m_window->update();
        update(m_time, m_delta_time);
        prerender();
        render();
        postrender();
    }

    exit();
}

void Application::prerender()
{
    vk::frame& current_frame = ctx.get_current_frame();

    vk::swapchain& swapchain = *m_rhi->get_swapchain();

    VK_CHECK(vkWaitForFences(ctx.device, 1, &current_frame.fence_queue_submitted, true, UINT64_MAX));
    VK_CHECK(vkResetFences(ctx.device, 1, &current_frame.fence_queue_submitted));

    swapchain.acquire_next_image(current_frame.semaphore_swapchain_acquire);
    
    VkCommandBufferBeginInfo cmdBufferBeginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    current_frame.cmd_buffer.begin();
    swapchain.color_attachments[swapchain.current_backbuffer_idx].transition(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

}

void Application::postrender()
{
    vk::frame& current_frame = ctx.get_current_frame();
    vk::swapchain& swapchain = *m_rhi->get_swapchain();

    /* Transition to present */
    swapchain.color_attachments[swapchain.current_backbuffer_idx].transition(current_frame.cmd_buffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_NONE);
    VK_CHECK(vkEndCommandBuffer(current_frame.cmd_buffer));

    // Submit commands for the GPU to work on the current backbuffer
    // Has to wait for the swapchain image to be acquired before beginning, we wait on imageAcquired semaphore.
    // Signals a renderComplete semaphore to let the next operation know that it finished
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = current_frame.cmd_buffer.ptr();
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &current_frame.semaphore_swapchain_acquire;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &current_frame.smp_queue_submitted;

    vkQueueSubmit(ctx.device.graphics_queue, 1, &submit_info, current_frame.fence_queue_submitted);

    // Present work
    // Waits for the GPU queue to finish execution before presenting, we wait on renderComplete semaphore
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain.vk_swapchain;
    present_info.pImageIndices = &swapchain.current_backbuffer_idx;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &current_frame.smp_queue_submitted;

    vkQueuePresentKHR(ctx.device.graphics_queue, &present_info);

    ctx.frame_count++;
    ctx.update_frame_index();
}

double Application::get_time_secs() 
{ 
    return m_window->GetTime(); 
}

void Application::on_window_resize() 
{
    LOG_INFO("Windows Resize");

    ctx.swapchain->reinitialize();
}

void Application::on_mouse_down(MouseEvent event)
{
    m_event_manager->mouse_event = event;

    if (event.b_first_click)
    {
        LOG_INFO("Mouse Event: First Click");
    }
    if (event.b_lmb_click)
    {
        LOG_INFO("LMB clicked ({}, {})", event.curr_click_px, event.curr_click_py);
    }
    else if (event.b_mmb_click)
    {
        LOG_INFO("MMB clicked ({}, {})", event.curr_click_px, event.curr_click_py);
    }
    if (event.b_rmb_click)
    {
        LOG_INFO("RMB clicked ({}, {})", event.curr_click_px, event.curr_click_py);
    }
    if (event.b_wheel_scroll)
    {
        LOG_INFO("Wheel scroll Z Delta ({})", event.wheel_zdelta);
    }
}

void Application::on_mouse_up(MouseEvent event)
{
    m_event_manager->mouse_event = event;

    LOG_INFO("Mouse released ({}, {})", event.curr_click_px, event.curr_click_py);
}

void Application::on_mouse_move(MouseEvent event)
{
    m_event_manager->mouse_event = event;

    if (event.b_lmb_click)
    {
        LOG_INFO("LMB dragging ({}, {})", event.curr_pos_x, event.curr_pos_y);
    }

    if (event.b_mmb_click)
    {
        LOG_INFO("MMB dragging ({}, {})", event.curr_pos_x, event.curr_pos_y);
    }

    if (event.b_rmb_click)
    {
        LOG_INFO("RMB dragging ({}, {})", event.curr_pos_x, event.curr_pos_y);
    }
}

bool Application::get_key_state(Key key)
{
    return m_window->AsyncKeyState(key);
}

void Application::on_key_event(KeyEvent event)
{
    m_event_manager->key_event.pressed = event.pressed;
    m_event_manager->key_event.key = event.key;
}
