#include <Crow/Log.hpp>
#include <Crow/Window.hpp>

#include <Crow/Renderer.hpp>

int main() {
    crow::WindowBuilder builder;

    auto window_ret =
        builder.SetTitle("Test Vulkan").SetWindowedSize(800, 600).Build();

    if (!window_ret) {
        crow::println("Failed to create window");
        return -1;
    }

    auto window = std::move(window_ret.value());

    while (!window.ShouldClose()) {
        window.Update();

        crow::renderer.StartFrame();

        // Draw here

        crow::renderer.SubmitFrame();
    }
}