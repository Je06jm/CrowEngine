#ifndef CROW_WINDOW_HPP
#define CROW_WINDOW_HPP

#include <Crow/Vulkan.hpp>
#include <VkBootstrap.h>
#include <optional>
#include <string>
#include <tuple>

namespace crow {

class Window {
  private:
    GLFWwindow* window;

    std::string title;
    size_t width, height;
    bool fullscreen;

  public:
    Window(const std::string& title, size_t width, size_t height,
           bool fullscreen);
    ~Window();

    void SetTitle(const std::string& title);
    inline auto GetTitle() const { return title; }

    void SetSize(size_t width, size_t height);
    inline std::tuple<size_t, size_t> GetSize() const {
        return {width, height};
    }

    std::tuple<size_t, size_t> GetFullscreenSize() const;

    void SetFullscreen(bool fullscreen);
    inline auto GetFullscreen() const { return fullscreen; }

    void Update();

    bool ShouldClose();

    inline bool Valid() const { return window; }
};

class WindowBuilder {
  private:
    std::string title = "CrowEngine";
    size_t width = 1280;
    size_t height = 720;
    bool fullscreen = false;

  public:
    inline WindowBuilder& SetTitle(const std::string& title) {
        this->title = title;
        return *this;
    }

    inline WindowBuilder& SetWindowedSize(size_t width, size_t height) {
        this->width = width;
        this->height = height;
        fullscreen = false;
        return *this;
    }

    WindowBuilder& SetFullscreenSize(size_t width = 0, size_t height = 0);

    inline std::optional<Window> Build() const {
        Window window(title, width, height, fullscreen);

        if (!window.Valid()) {
            return {};
        }

        return window;
    }
};

} // namespace crow

#endif