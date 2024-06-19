#ifndef CROW_RENDERER_HPP
#define CROW_RENDERER_HPP

#include <Crow/Vulkan.hpp>
#include <cstddef>
#include <memory>

namespace crow {

class Renderer {
    uint32_t current_framebuffer;

  public:
    void StartFrame();
    void SubmitFrame();

    inline VkFramebuffer GetCurrentFramebuffer() const {
        return vk_framebuffers[current_framebuffer];
    }
};

inline Renderer renderer;

} // namespace crow

#endif