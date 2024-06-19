#ifndef CROW_VULKAN_HPP
#define CROW_VULKAN_HPP

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

#include <vector>

namespace crow {

inline vkb::Instance vkb_instance;
inline vkb::Device vkb_device;

inline VkSurfaceKHR vk_surface;

inline VkQueue vk_graphics_queue;
inline VkQueue vk_compute_queue;
inline VkQueue vk_present_queue;
inline VkQueue vk_transfer_queue;

inline vkb::Swapchain vkb_swapchain;

inline size_t vk_frame_index = 0;

inline std::vector<VkImageView> vk_image_views;
inline std::vector<VkRenderPass> vk_render_passes;
inline std::vector<VkFramebuffer> vk_framebuffers;

inline VkCommandPool vk_graphics_cmd_pool;
inline VkCommandPool vk_compute_cmd_pool;

inline std::vector<VkCommandBuffer> vk_cmd_graphics;
inline std::vector<VkCommandBuffer> vk_cmd_compute;

inline std::vector<VkSemaphore> vk_render_finished_semaphores;
inline std::vector<VkSemaphore> vk_image_available_semaphores;
inline std::vector<VkFence> vk_graphics_flight_fences;

inline std::vector<VkSemaphore> vk_compute_finished_semaphores;
inline std::vector<VkFence> vk_compute_flight_fences;

} // namespace crow

#endif