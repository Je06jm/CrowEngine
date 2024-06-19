#include <Crow/Renderer.hpp>

#include <Crow/Vulkan.hpp>

namespace crow {

void Renderer::StartFrame() {
    vkWaitForFences(vkb_device.device, 1,
                    &vk_graphics_flight_fences[vk_frame_index], VK_TRUE,
                    UINT64_MAX);
    vkResetFences(vkb_device.device, 1,
                  &vk_graphics_flight_fences[vk_frame_index]);

    vkWaitForFences(vkb_device.device, 1,
                    &vk_compute_flight_fences[vk_frame_index], VK_TRUE,
                    UINT64_MAX);
    vkResetFences(vkb_device.device, 1,
                  &vk_compute_flight_fences[vk_frame_index]);

    vkAcquireNextImageKHR(vkb_device.device, vkb_swapchain.swapchain,
                          UINT64_MAX,
                          vk_image_available_semaphores[vk_frame_index],
                          VK_NULL_HANDLE, &current_framebuffer);

    // TODO Detect resizes and recreate swapchain

    vkResetCommandBuffer(vk_cmd_graphics[vk_frame_index], 0);
    vkResetCommandBuffer(vk_cmd_graphics[vk_frame_index], 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(vk_cmd_graphics[vk_frame_index], &begin_info);

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = vk_render_passes[vk_frame_index];
    render_pass_info.framebuffer = vk_framebuffers[vk_frame_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = vkb_swapchain.extent;

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    clear_color.depthStencil.depth = 0.0f;
    clear_color.depthStencil.stencil = 0;

    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(vk_cmd_graphics[vk_frame_index], &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkBeginCommandBuffer(vk_cmd_compute[vk_frame_index], &begin_info);
}

void Renderer::SubmitFrame() {
    vkEndCommandBuffer(vk_cmd_compute[vk_frame_index]);

    vkCmdEndRenderPass(vk_cmd_graphics[vk_frame_index]);
    vkEndCommandBuffer(vk_cmd_graphics[vk_frame_index]);

    {
        VkSubmitInfo compute_submit_info{};
        compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        compute_submit_info.commandBufferCount = 1;
        compute_submit_info.pCommandBuffers = &vk_cmd_compute[vk_frame_index];
        compute_submit_info.signalSemaphoreCount = 1;
        compute_submit_info.pSignalSemaphores =
            &vk_compute_finished_semaphores[vk_frame_index];

        vkQueueSubmit(vk_compute_queue, 1, &compute_submit_info,
                      vk_compute_flight_fences[vk_frame_index]);
    }

    {
        VkSemaphore wait_semaphores[] = {
            vk_compute_finished_semaphores[vk_frame_index],
            vk_image_available_semaphores[vk_frame_index]};

        VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo graphics_submit_info{};
        graphics_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        graphics_submit_info.commandBufferCount = 1;
        graphics_submit_info.pCommandBuffers = &vk_cmd_graphics[vk_frame_index];
        graphics_submit_info.waitSemaphoreCount = 2;
        graphics_submit_info.pWaitSemaphores = wait_semaphores;
        graphics_submit_info.pWaitDstStageMask = wait_stages;
        graphics_submit_info.signalSemaphoreCount = 1;
        graphics_submit_info.pSignalSemaphores =
            &vk_render_finished_semaphores[vk_frame_index];

        vkQueueSubmit(vk_graphics_queue, 1, &graphics_submit_info,
                      vk_graphics_flight_fences[vk_frame_index]);
    }

    {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores =
            &vk_render_finished_semaphores[vk_frame_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &vkb_swapchain.swapchain;
        present_info.pImageIndices = &current_framebuffer;

        vkQueuePresentKHR(vk_present_queue, &present_info);
    }

    vk_frame_index++;
    vk_frame_index %= vkb_swapchain.image_count;
}

} // namespace crow