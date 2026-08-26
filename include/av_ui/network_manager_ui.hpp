#pragma once
#include <iostream>
#include <chrono>
#include <future>
#include <string>
#include <vector>
#include <av_net/network_manager.hpp>
#include <av_root/ui_component.hpp>
#include <av_root/root.hpp>
#include <av_root/av_request.hpp>

namespace avUi
{
    /// @brief Application host: owns the GLFW window + ImGui context and runs the
    ///        event/render loop. It builds a retained avR::UiComponent tree and
    ///        draws it each frame, but is deliberately NOT a UiComponent itself —
    ///        it is the root driver of the tree, not a node within it.
    class NetworkManagerUi
    {
    public:
        NetworkManagerUi();
        ~NetworkManagerUi();

        /// @brief Opens the arvis GUI window and runs the event/render loop until
        ///        the user closes the window.
        void run();

    private:
        static constexpr int _min_window_w = 900;
        static constexpr int _min_window_h = 600;
        static constexpr float _restored_size_ratio = .85f;

        const double fps;
        int width;
        int height;
        int _pos_x;
        int _pos_y;
        bool _frame_in_flight;
        avR::AvRoot avRoot;
        GLFWwindow *window;
        GLFWmonitor *monitor;
        std::unique_ptr<avR::UiComponent> _root_ui;

        void compute_initial_geometry();
        void render_frame();
        static void on_window_refresh(GLFWwindow *window);
        static void on_framebuffer_size(GLFWwindow *window, int w, int h);
    };
} // namespace avUi
