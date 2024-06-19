#include <Crow/Window.hpp>

#include <Crow/Log.hpp>

#include <cstdlib>

struct GLFWLifetime {
    GLFWLifetime() {
        if (!glfwInit()) {
            crow::println("Cannot initialize GLFW");
            std::exit(EXIT_FAILURE);
        }

        if (!glfwVulkanSupported()) {
            crow::println("Vulkan is not supported");
            std::exit(EXIT_FAILURE);
        }
    }

    ~GLFWLifetime() { glfwTerminate(); }
};

GLFWLifetime glfw_lifetime;

namespace crow {

Window::Window(const std::string& title, size_t width, size_t height,
               bool fullscreen)
    : title{title}, width{width}, height{height}, fullscreen{fullscreen} {

    {
        uint32_t count;
        const char** extentions = glfwGetRequiredInstanceExtensions(&count);
        vkb::InstanceBuilder builder;
        builder.set_engine_name("CrowEngine")
            .set_app_name(title.c_str())
#ifdef CROW_DEBUG
            .request_validation_layers()
#endif
            .set_debug_callback(
                [](VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                   void*) -> VkBool32 {
                    auto severity =
                        vkb::to_string_message_severity(message_severity);
                    auto type = vkb::to_string_message_type(message_type);
                    println("Vulkan Debug: [{}: {}] {}", severity, type,
                            callback_data->pMessage);
                    return VK_FALSE;
                });

        for (uint32_t i = 0; i < count; i++) {
            builder.enable_extension(extentions[i]);
        }

        auto inst_ret = builder.build();

        if (!inst_ret) {
            println("Failed to create Vulkan instance: {}",
                    inst_ret.error().message());
            return;
        }

        vkb_instance = inst_ret.value();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (fullscreen) {
        window = glfwCreateWindow(int(width), int(height), title.c_str(),
                                  glfwGetPrimaryMonitor(), nullptr);
    } else {
        window = glfwCreateWindow(int(width), int(height), title.c_str(),
                                  nullptr, nullptr);
    }

    auto DestroyWindow = [&]() {
        glfwDestroyWindow(window);
        window = nullptr;

        vkb::destroy_swapchain(vkb_swapchain);
        vkb::destroy_surface(vkb_instance, vk_surface);
        vkb::destroy_device(vkb_device);
        vkb::destroy_instance(vkb_instance);
    };

    if (glfwCreateWindowSurface(vkb_instance.instance, window, nullptr,
                                &vk_surface)) {
        println("Failed to create window surface");
        DestroyWindow();
        return;
    }

    {
        vkb::PhysicalDeviceSelector selector{vkb_instance};

        auto phys_ret = selector.set_surface(vk_surface)
                            .set_minimum_version(1, 1)
                            .require_dedicated_transfer_queue()
                            .select();

        if (!phys_ret) {
            println("Failed to select physical Vulkan device: {}",
                    phys_ret.error().message());
            DestroyWindow();
            return;
        }

        auto phys = phys_ret.value();

        phys.enable_extension_if_present(
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        phys.enable_extension_if_present(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        phys.enable_extension_if_present(VK_EXT_MESH_SHADER_EXTENSION_NAME);

        vkb::DeviceBuilder device_builder{phys};
        auto dev_ret = device_builder.build();

        if (!dev_ret) {
            println("Failed to create Vulkan device: {}",
                    dev_ret.error().message());
            DestroyWindow();
            return;
        }

        vkb_device = dev_ret.value();
    }

    {
        auto graphics_queue_ret =
            vkb_device.get_queue(vkb::QueueType::graphics);
        if (!graphics_queue_ret) {
            println("Failed to get graphics queue: {}",
                    graphics_queue_ret.error().message());
            DestroyWindow();
            return;
        }

        vk_graphics_queue = graphics_queue_ret.value();

        auto compute_queue_ret = vkb_device.get_queue(vkb::QueueType::compute);
        if (!compute_queue_ret) {
            println("Failed to get compute queue: {}",
                    compute_queue_ret.error().message());
            DestroyWindow();
            return;
        }

        vk_compute_queue = compute_queue_ret.value();

        auto present_queue_ret = vkb_device.get_queue(vkb::QueueType::present);
        if (!present_queue_ret) {
            println("Failed to get present queue: {}",
                    present_queue_ret.error().message());
            DestroyWindow();
            return;
        }

        vk_present_queue = present_queue_ret.value();

        auto transfer_queue_ret =
            vkb_device.get_queue(vkb::QueueType::transfer);
        if (!transfer_queue_ret) {
            println("Failed to get transfer queue: {}",
                    transfer_queue_ret.error().message());
            DestroyWindow();
            return;
        }

        vk_transfer_queue = transfer_queue_ret.value();
    }

    {
        vkb::SwapchainBuilder swapchain_builder{vkb_device};

        auto swapchain_ret = swapchain_builder.use_default_format_selection()
                                 .use_default_image_usage_flags()
                                 .use_default_present_mode_selection()
                                 .build();

        if (!swapchain_ret) {
            println("Failed to create swapchain: {}",
                    swapchain_ret.error().message());
            DestroyWindow();
            return;
        }

        vkb_swapchain = swapchain_ret.value();
    }

    {
        auto image_views_ret = vkb_swapchain.get_image_views();

        if (!image_views_ret) {
            println("Failed to get image views from swapchain: {}",
                    image_views_ret.error().message());
            DestroyWindow();
            return;
        }

        vk_image_views = image_views_ret.value();
    }

    {
        for (uint32_t i = 0; i < vkb_swapchain.image_count; i++) {
            VkAttachmentDescription color_attachment{};
            color_attachment.format = vkb_swapchain.image_format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment_ref{};
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout =
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment_ref;

            VkRenderPass render_pass;

            VkRenderPassCreateInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.attachmentCount = 1;
            render_pass_info.pAttachments = &color_attachment;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;

            if (vkCreateRenderPass(vkb_device.device, &render_pass_info,
                                   vkb_device.allocation_callbacks,
                                   &render_pass) != VK_SUCCESS) {
                println("Could not create swapchain render pass");
                DestroyWindow();
                return;
            }

            vk_render_passes.push_back(render_pass);
        }
    }

    for (uint32_t i = 0; i < vkb_swapchain.image_count; i++) {
        VkImageView attachments[] = {vk_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

        framebuffer_info.renderPass = vk_render_passes[i];
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = vkb_swapchain.extent.width;
        framebuffer_info.height = vkb_swapchain.extent.height;
        framebuffer_info.layers = 1;

        VkFramebuffer framebuffer;

        if (vkCreateFramebuffer(vkb_device.device, &framebuffer_info,
                                vkb_device.allocation_callbacks,
                                &framebuffer) != VK_SUCCESS) {
            println("Could not create swapchain framebuffer");
            DestroyWindow();
            return;
        }

        vk_framebuffers.push_back(framebuffer);
    }

    {
        VkCommandPoolCreateInfo graphics_pool_info{};
        graphics_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        graphics_pool_info.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphics_pool_info.queueFamilyIndex =
            vkb_device.get_queue_index(vkb::QueueType::graphics).value();

        if (vkCreateCommandPool(vkb_device.device, &graphics_pool_info,
                                vkb_device.allocation_callbacks,
                                &vk_graphics_cmd_pool) != VK_SUCCESS) {
            println("Could not create graphics command pool");
            DestroyWindow();
            return;
        }
    }

    {
        VkCommandPoolCreateInfo compute_pool_info{};
        compute_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

        compute_pool_info.flags =
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        compute_pool_info.queueFamilyIndex =
            vkb_device.get_queue_index(vkb::QueueType::compute).value();

        if (vkCreateCommandPool(vkb_device.device, &compute_pool_info,
                                vkb_device.allocation_callbacks,
                                &vk_compute_cmd_pool) != VK_SUCCESS) {
            println("Could not create compute command pool");
            DestroyWindow();
            return;
        }
    }

    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = vk_graphics_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = vkb_swapchain.image_count;

        vk_cmd_graphics.resize(vkb_swapchain.image_count);

        if (vkAllocateCommandBuffers(vkb_device.device, &alloc_info,
                                     vk_cmd_graphics.data()) != VK_SUCCESS) {
            println("Could not allocate graphics command buffer");
            DestroyWindow();
            return;
        }
    }

    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = vk_compute_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = vkb_swapchain.image_count;

        vk_cmd_compute.resize(vkb_swapchain.image_count);

        if (vkAllocateCommandBuffers(vkb_device.device, &alloc_info,
                                     vk_cmd_compute.data()) != VK_SUCCESS) {
            println("Could not allocate compute command buffer");
            DestroyWindow();
            return;
        }
    }

    vk_render_finished_semaphores.resize(vkb_swapchain.image_count);
    vk_image_available_semaphores.resize(vkb_swapchain.image_count);
    vk_graphics_flight_fences.resize(vkb_swapchain.image_count);

    vk_compute_finished_semaphores.resize(vkb_swapchain.image_count);
    vk_compute_flight_fences.resize(vkb_swapchain.image_count);

    for (uint32_t i = 0; i < vkb_swapchain.image_count; i++) {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(vkb_device.device, &semaphore_info,
                              vkb_device.allocation_callbacks,
                              &vk_render_finished_semaphores[i]) !=
            VK_SUCCESS) {
            println("Could not create swapchain semaphore");
            DestroyWindow();
            return;
        }

        if (vkCreateSemaphore(vkb_device.device, &semaphore_info,
                              vkb_device.allocation_callbacks,
                              &vk_image_available_semaphores[i]) !=
            VK_SUCCESS) {
            println("Could not create swapchain semaphore");
            DestroyWindow();
            return;
        }

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(vkb_device.device, &fence_info,
                          vkb_device.allocation_callbacks,
                          &vk_graphics_flight_fences[i]) != VK_SUCCESS) {
            println("Could not create swapchain fence");
            DestroyWindow();
            return;
        }

        if (vkCreateSemaphore(vkb_device.device, &semaphore_info,
                              vkb_device.allocation_callbacks,
                              &vk_compute_finished_semaphores[i]) !=
            VK_SUCCESS) {
            println("Could not create compute semaphore");
            DestroyWindow();
            return;
        }

        if (vkCreateFence(vkb_device.device, &fence_info,
                          vkb_device.allocation_callbacks,
                          &vk_compute_flight_fences[i]) != VK_SUCCESS) {
            println("Could not create compute fence");
            DestroyWindow();
            return;
        }
    }
}

Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;

        vkDeviceWaitIdle(vkb_device.device);

        for (auto& fence : vk_compute_flight_fences) {
            vkDestroyFence(vkb_device.device, fence,
                           vkb_device.allocation_callbacks);
        }

        for (auto& semaphore : vk_compute_finished_semaphores) {
            vkDestroySemaphore(vkb_device.device, semaphore,
                               vkb_device.allocation_callbacks);
        }

        for (auto& fence : vk_graphics_flight_fences) {
            vkDestroyFence(vkb_device.device, fence,
                           vkb_device.allocation_callbacks);
        }

        for (auto& semaphore : vk_image_available_semaphores) {
            vkDestroySemaphore(vkb_device.device, semaphore,
                               vkb_device.allocation_callbacks);
        }

        for (auto& semaphore : vk_render_finished_semaphores) {
            vkDestroySemaphore(vkb_device.device, semaphore,
                               vkb_device.allocation_callbacks);
        }

        vkDestroyCommandPool(vkb_device.device, vk_compute_cmd_pool,
                             vkb_device.allocation_callbacks);
        vkDestroyCommandPool(vkb_device.device, vk_graphics_cmd_pool,
                             vkb_device.allocation_callbacks);

        for (auto& framebuffer : vk_framebuffers) {
            vkDestroyFramebuffer(vkb_device.device, framebuffer,
                                 vkb_device.allocation_callbacks);
        }

        for (auto& render_pass : vk_render_passes) {
            vkDestroyRenderPass(vkb_device.device, render_pass,
                                vkb_device.allocation_callbacks);
        }

        vkb_swapchain.destroy_image_views(vk_image_views);

        vkb::destroy_swapchain(vkb_swapchain);
        vkb::destroy_surface(vkb_instance, vk_surface);
        vkb::destroy_device(vkb_device);
        vkb::destroy_instance(vkb_instance);
    }
}

void Window::SetTitle(const std::string& title) {
    this->title = title;

    if (window) {
        glfwSetWindowTitle(window, title.c_str());
    }
}

void Window::SetSize(size_t width, size_t height) {
    this->width = width;
    this->height = height;

    if (window) {
        glfwSetWindowSize(window, int(width), int(height));
    }
}

std::tuple<size_t, size_t> Window::GetFullscreenSize() const {
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);

    return {size_t(mode->width), size_t(mode->height)};
}

void Window::SetFullscreen(bool fullscreen) {
    this->fullscreen = fullscreen;

    if (window) {
        GLFWmonitor* monitor = nullptr;
        int x = 0, y = 0;

        if (fullscreen) {
            monitor = glfwGetPrimaryMonitor();

            auto mode = glfwGetVideoMode(monitor);

            x = mode->width / 2 - (int(width) / 2);
            y = mode->height / 2 - (int(height) / 2);
        }

        glfwSetWindowMonitor(window, monitor, x, y, int(width), int(height),
                             GLFW_DONT_CARE);
    }
}

void Window::Update() {
    if (window) {
        glfwPollEvents();
    }
}

bool Window::ShouldClose() {
    if (window) {
        return glfwWindowShouldClose(window);
    }
    return true;
}

WindowBuilder& WindowBuilder::SetFullscreenSize(size_t width, size_t height) {
    auto monitor = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(monitor);

    if (width == 0) {
        width = mode->width;
    }

    if (height == 0) {
        height = mode->height;
    }

    this->width = width;
    this->height = height;
    fullscreen = true;

    return *this;
}

} // namespace crow