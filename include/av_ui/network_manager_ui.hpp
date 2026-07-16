#pragma once
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <future>
#include <string>
#include <av_net/network_manager.hpp>
#include <av_root/root.hpp>

namespace avUi
{
    class NetworkManagerUi
    {
    public:
        NetworkManagerUi();
        ~NetworkManagerUi();

        /// @brief Opens the arvis GUI window and runs the event/render loop until
        ///        the user closes the window.
        /// @return process exit code (0 on clean exit, non-zero on init failure).
        int start() const;

    private:
        avR::AvRoot m_root;
    };
}
