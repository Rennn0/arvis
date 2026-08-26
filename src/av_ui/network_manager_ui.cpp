#include <av_ui/network_manager_ui.hpp>
#include <av_ui/logo_icon.hpp>
#include <av_ui/root_ui.hpp>
#include <GLFW/glfw3native.h>

namespace avUi
{
    NetworkManagerUi::NetworkManagerUi()
        : fps(1. / 60.), width(0), height(0), _pos_x(0), _pos_y(0), avRoot("NetworkManagerUI"), window(nullptr),
          monitor(nullptr), _frame_in_flight(false)
    {
        if (!glfwInit())
        {
            const char *desc;
            glfwGetError(&desc);
            this->avRoot.log_error(desc);
            throw std::runtime_error("glfw init failed");
        }

        this->monitor = glfwGetPrimaryMonitor();
        this->compute_initial_geometry();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
        glfwWindowHint(GLFW_POSITION_X, this->_pos_x);
        glfwWindowHint(GLFW_POSITION_Y, this->_pos_y);
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
        glfwSwapInterval(1);
        glfwSetWindowSizeLimits(this->window, this->_min_window_w, this->_min_window_h, GLFW_DONT_CARE, GLFW_DONT_CARE);

        GLFWimage icons[avUi::logo_icon_count]{};

        for (int i = 0; i < avUi::logo_icon_count; ++i)
            icons[i] = GLFWimage{avUi::logo_icon_images[i].width, avUi::logo_icon_images[i].height,
                                 avUi::logo_icon_images[i].pixels};
        glfwSetWindowIcon(this->window, avUi::logo_icon_count, icons);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(this->window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        this->_root_ui = std::make_unique<avUi::RootUi>("root");
        glfwSetWindowUserPointer(this->window, this);
        glfwSetWindowRefreshCallback(this->window, &NetworkManagerUi::on_window_refresh);
        glfwSetFramebufferSizeCallback(this->window, &NetworkManagerUi::on_framebuffer_size);

        while (!glfwWindowShouldClose(this->window))
        {
            glfwWaitEventsTimeout(this->fps);
            this->render_frame();
        }

        glfwSetFramebufferSizeCallback(this->window, nullptr);
        glfwSetWindowRefreshCallback(this->window, nullptr);
        glfwSetWindowUserPointer(this->window, nullptr);

        this->_root_ui.reset();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(this->window);
        this->window = nullptr;
    }
    void NetworkManagerUi::compute_initial_geometry()
    {
        int areaX = 0;
        int areaY = 0;
        int areaW = 0;
        int areaH = 0;
        if (this->monitor)
            glfwGetMonitorWorkarea(this->monitor, &areaX, &areaY, &areaW, &areaH);
        if (areaW <= 0 || areaH <= 0)
        {
            const GLFWvidmode *mode = this->monitor ? glfwGetVideoMode(this->monitor) : nullptr;
            areaX = 0;
            areaY = 0;
            areaW = mode ? mode->width : this->_min_window_w;
            areaH = mode ? mode->height : this->_min_window_h;
        }
        this->width = std::clamp(static_cast<int>(areaW * this->_restored_size_ratio), this->_min_window_w, areaW);
        this->height = std::clamp(static_cast<int>(areaH * this->_restored_size_ratio), this->_min_window_h, areaH);
        this->_pos_x = areaX + (areaW - this->width) / 2;
        this->_pos_y = areaY + (areaH - this->height) / 2;
    }
    void NetworkManagerUi::render_frame()
    {
        if (!this->window || !this->_root_ui || this->_frame_in_flight)
            return;

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(this->window, &fb_width, &fb_height);

        if (fb_width <= 0 || fb_height <= 0)
            return;
        this->_frame_in_flight = true;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        this->_root_ui->draw();
        ImGui::Render();

        glViewport(0, 0, fb_width, fb_height);

        const ImVec4 &bg = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        glClearColor(bg.x, bg.y, bg.z, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(this->window);

        this->_frame_in_flight = false;
    }
    void NetworkManagerUi::on_window_refresh(GLFWwindow *window)
    {
        if (NetworkManagerUi *self = static_cast<NetworkManagerUi *>(glfwGetWindowUserPointer(window)))
            self->render_frame();
    }
    void NetworkManagerUi::on_framebuffer_size(GLFWwindow *window, int w, int h)
    {
        (void)w;
        (void)h;
        if (NetworkManagerUi *self = static_cast<NetworkManagerUi *>(glfwGetWindowUserPointer(window)))
            self->render_frame();
    }
} // namespace avUi
