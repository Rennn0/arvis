#include <av_ui/network_manager_ui.hpp>
#include <av_ui/logo_icon.hpp>
#include <av_ui/root_ui.hpp>

// Including <GLFW/glfw3.h> without GLFW_INCLUDE_NONE also pulls in the platform
// OpenGL header, giving us the GL 1.1 calls (glViewport/glClear/...) we use for
// the frame clear — portably across Windows/Linux/macOS.

// Win32-only: reach the native HWND so DWM can paint the OS title bar (caption +
// min/max/close) in the dark theme. GLFW doesn't expose this. Guarded so the
// Linux/macOS builds are untouched; dwmapi is auto-linked via #pragma comment.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20 // Win10 20H1+ / Win11; value fixed by DWM
#endif
#endif

namespace avUi
{
    NetworkManagerUi::NetworkManagerUi()
        : fps(1. / 60.), width(0), height(0), avRoot("NetworkManagerUI"), window(nullptr), monitor(nullptr)
    {
        if (!glfwInit())
        {
            const char *desc;
            glfwGetError(&desc);
            this->avRoot.log_error(desc);
            throw std::runtime_error("glfw init failed");
        }

        this->monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        this->width = mode->width;
        this->height = mode->height;
    }

    NetworkManagerUi::~NetworkManagerUi()
    {
        this->avRoot.log_info("dctor");
        glfwTerminate();
        this->window = nullptr;
        this->monitor = nullptr;
    }

    void NetworkManagerUi::run()
    {
        this->window = glfwCreateWindow(this->width, this->height, "Arvis", nullptr, nullptr);
        if (!this->window)
        {
            const char *desc;
            glfwGetError(&desc);
            this->avRoot.log_error(desc ? desc : "glfwCreateWindow failed");
            throw std::runtime_error("glfw create window failed");
        }

        glfwMakeContextCurrent(this->window);
        glfwMaximizeWindow(this->window);
        glfwSwapInterval(1);

        GLFWimage icons[avUi::logo_icon_count]{};

        for (int i = 0; i < avUi::logo_icon_count; ++i)
            icons[i] = GLFWimage{avUi::logo_icon_images[i].width, avUi::logo_icon_images[i].height,
                                 avUi::logo_icon_images[i].pixels};
        glfwSetWindowIcon(this->window, avUi::logo_icon_count, icons);

#ifdef _WIN32
        {
            HWND hwnd = glfwGetWin32Window(this->window);
            BOOL useDark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
        }
#endif

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(this->window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        std::unique_ptr<avR::UiComponent> rootUi = std::make_unique<avUi::RootUi>("root");

        double lastFrame = 0.;
        while (!glfwWindowShouldClose(this->window))
        {
            glfwWaitEventsTimeout(this->fps);

            double now = glfwGetTime();
            double delta = now - lastFrame;
            if (delta < this->fps)
                continue;
            lastFrame = now;

            this->check_keyboard_events();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            rootUi->draw();
            ImGui::Render();

            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
    }

    void NetworkManagerUi::check_keyboard_events()
    {
        if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(this->window, true);
        }
    }

} // namespace avUi
