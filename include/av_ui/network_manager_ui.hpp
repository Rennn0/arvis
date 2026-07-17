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
        const double fps;
        int width;
        int height;
        avR::AvRoot avRoot;
        GLFWwindow *window;
        GLFWmonitor *monitor;

        void check_keyboard_events();
    };
} // namespace avUi
