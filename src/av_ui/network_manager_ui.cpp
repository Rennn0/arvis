#include <av_ui/network_manager_ui.hpp>
#include <av_ui/request_list_ui.hpp>
#include <av_ui/detailed_request_view_ui.hpp>
#include <av_ui/logo_icon.hpp>
#include <av_root/av_button.hpp>
#include <av_root/av_custom.hpp>
#include <av_root/av_div.hpp>

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

        glfwMakeContextCurrent(this->window);
        glfwMaximizeWindow(this->window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(this->window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        std::shared_ptr<avR::UiComponent> root = std::make_shared<avR::AvDiv>("root", avR::AvDiv::Config{});
        this->setup_root(root);

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

            const ImGuiViewport *viewPort = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewPort->WorkPos);
            ImGui::SetNextWindowSize(viewPort->WorkSize);
            const ImGuiWindowFlags hostFlag = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                              ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                              ImGuiWindowFlags_MenuBar;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("##host", nullptr, hostFlag);
            ImGui::PopStyleVar();
            this->setup_menu();
            root->draw();
            ImGui::End();

            ImGui::Render();
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.07f, 0.07f, 0.09f, 1.0f);
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

    void NetworkManagerUi::setup_menu()
    {
        bool fontSliderActive = false;
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("menu"))
            {
                if (ImGui::MenuItem("open", "Ctrl 1"))
                {
                    this->avRoot.log_info("open clicked");
                }

                ImGui::Separator();
                if (ImGui::MenuItem("exit", "Esc"))
                {
                    this->avRoot.log_info("close clicked");
                    glfwSetWindowShouldClose(this->window, true);
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("settings"))
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("scale");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(140.f);
                ImGui::SliderFloat("##scale", &this->fontScale, 0.75f, 3.0f, "%.2fx");
                fontSliderActive = ImGui::IsItemActive();

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (!fontSliderActive)
        {
            ImGui::GetIO().FontGlobalScale = this->fontScale;
        }

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_1, ImGuiInputFlags_RouteGlobal))
        {
            this->avRoot.log_info("open clicked");
            // #TODO
        }
    }

    void NetworkManagerUi::setup_root(std::shared_ptr<avR::UiComponent> &root)
    {
        using Dir = avR::AvDiv::Direction;
        std::shared_ptr<avUi::RequstListViewUi::RequestListState> requestListState =
            std::make_shared<avUi::RequstListViewUi::RequestListState>();
        avR::AvDiv::Config conf;
        avR::AvButton::Style style;

        conf.direction = Dir::Vertical;
        conf.size = ImVec2(0, 0);
        conf.padding = ImVec2(0, 0); // no inset on the whole layout
        conf.spacing = 20.f;
        conf.border = false;
        std::static_pointer_cast<avR::AvDiv>(root)->configure(conf);

        conf = {};
        conf.direction = Dir::Horizontal;
        conf.size = ImVec2(0, 44);
        conf.padding = ImVec2(16.0f, 0.0f);
        conf.spacing = 8.f;
        conf.background = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
        conf.border = false;
        avR::UiComponent *header = root->add_child(std::make_unique<avR::AvDiv>("header", conf));

        style.size = ImVec2(0.0f, 0.0f);
        style.padding = ImVec2(14.0f, 5.0f);
        style.background = ImVec4(0.18f, 0.20f, 0.24f, 1.0f);
        style.hovered = ImVec4(0.24f, 0.28f, 0.34f, 1.0f);
        style.active = ImVec4(0.14f, 0.16f, 0.20f, 1.0f);
        style.text = ImVec4(0.90f, 0.92f, 0.95f, 1.0f);
        style.rounding = 4.f;
        style.border = 0.f;
        avR::UiComponent *addBtn = header->add_child(std::make_unique<avR::AvButton>("new request", style));
        addBtn->set_on_click(
            [state = requestListState]
            {
                avR::AvRequest req;
                req.id = state->next_request_id++;
                state->requests.push_back(std::move(req));
                // focus the new entry right away
                state->selected_request_index = state->requests.size() - 1;
            });

        conf = {};
        conf.direction = Dir::Horizontal;
        conf.size = ImVec2(0, 0);
        conf.padding = ImVec2(0, 0);
        conf.spacing = 0.f;
        conf.resizable = true; // draggable divider between sidebar and mid
        conf.splitter_thickness = 6.f;
        conf.resize_min = 150.f; // sidebar can't shrink below this
        conf.resize_max = 500.f; // ...or grow past this
        avR::UiComponent *body = root->add_child(std::make_unique<avR::AvDiv>("body", conf));

        conf = {};
        conf.direction = Dir::Vertical;
        conf.size = ImVec2(220, 0);
        conf.border = true;
        avR::UiComponent *sidebar = body->add_child(std::make_unique<avR::AvDiv>("sidebar", conf));
        sidebar->add_child(std::make_unique<avUi::RequstListViewUi>(requestListState, "request_list"));

        conf = {};
        conf.size = ImVec2(0, 0);
        conf.background = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
        avR::UiComponent *mid = body->add_child(std::make_unique<avR::AvDiv>("mid", conf));
        // Detail pane for whichever request is selected in the sidebar.
        mid->add_child(std::make_unique<avUi::DetailedRequestViewUi>(requestListState,"request_details"));
    }
} // namespace avUi
