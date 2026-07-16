#include <av_ui/network_manager_ui.hpp>

// Including <GLFW/glfw3.h> without GLFW_INCLUDE_NONE also pulls in the platform
// OpenGL header, giving us the GL 1.1 calls (glViewport/glClear/...) we use for
// the frame clear — portably across Windows/Linux/macOS.

namespace avUi
{
    void glfw_error_callback(int error, const char *description)
    {
        std::fprintf(stderr, "[glfw] error %d: %s\n", error, description);
    }

    const char *status_text(avNet::response_status s)
    {
        switch (s)
        {
        case avNet::response_status::Ok:
            return "Ok";
        case avNet::response_status::Failed:
            return "Failed";
        case avNet::response_status::Canceled:
            return "Canceled";
        }
        return "Unknown";
    }

    NetworkManagerUi::NetworkManagerUi() : m_root("NetworkManagerUI")
    {
    }

    NetworkManagerUi::~NetworkManagerUi()
    {
    }

    int NetworkManagerUi::start() const
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            this->m_root.log_error("[glfw] init failed");
            return 1;
        }

        // Request an OpenGL 3.0-capable context; matches "#version 130" GLSL below.
        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        GLFWwindow *window = glfwCreateWindow(1000, 700, "arvis", nullptr, nullptr);
        if (window == nullptr)
        {
            this->m_root.log_error("[glfw] window creation failed");
            glfwTerminate();
            return 1;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // vsync

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr; // don't litter an imgui.ini next to the exe
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        glfwSetKeyCallback(window,
                           [](GLFWwindow *window, int key, int scanCode, int action, int mods)
                           {
                               const avUi::NetworkManagerUi *self = static_cast<const NetworkManagerUi *>(glfwGetWindowUserPointer(window));

                               if (key == GLFW_KEY_F10 && action == GLFW_PRESS)
                                   self->m_root.log_info("F10");

                               if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
                                   self->m_root.log_info("DOWN");
                           });

        // One network manager for the whole session (owns curl global init/cleanup).
        avNet::NetworkManager net;

        // --- UI state ------------------------------------------------------------
        char url[2048] = "https://api.xati.org/health";
        int method_idx = 0; // 0 = GET, 1 = POST
        std::future<avNet::http_result> pending;
        bool in_flight = false;
        avNet::http_result last;
        bool have_result = false;

        const ImVec4 clear_color(0.10f, 0.11f, 0.13f, 1.00f);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // Collect a finished request, if any.
            if (in_flight && pending.valid() &&
                pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                last = pending.get();
                have_result = true;
                in_flight = false;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Full-window panel.
            const ImGuiViewport *vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(vp->WorkPos);
            ImGui::SetNextWindowSize(vp->WorkSize);
            ImGui::Begin("arvis", nullptr,
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

            const ImGuiStyle &style = ImGui::GetStyle();

            ImGui::TextUnformatted("Request");
            ImGui::Separator();

            // URL field + Send button share one row and reflow with the window:
            // the button keeps a fixed width, the input stretches to fill the rest.
            const float send_w = 90.0f;
            const float input_w =
                ImGui::GetContentRegionAvail().x - send_w - style.ItemSpacing.x;
            ImGui::SetNextItemWidth(input_w > 100.0f ? input_w : 100.0f);
            const bool submit_via_enter =
                ImGui::InputText("##url", url, sizeof(url),
                                 ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::SameLine();
            ImGui::BeginDisabled(in_flight);
            const bool clicked = ImGui::Button("Send", ImVec2(send_w, 0.0f));
            ImGui::EndDisabled();

            ImGui::RadioButton("GET", &method_idx, 0);
            ImGui::SameLine();
            ImGui::RadioButton("POST", &method_idx, 1);
            if (in_flight)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("sending...");
            }

            if ((clicked || submit_via_enter) && !in_flight && url[0] != '\0')
            {
                const avNet::request_method method =
                    method_idx == 0 ? avNet::request_method::get
                                    : avNet::request_method::post;
                // Copy the URL so the worker doesn't race the UI-editable buffer.
                const std::string target(url);
                pending = std::async(std::launch::async,
                                     [&net, method, target]()
                                     {
                                         return method == avNet::request_method::get ? net.get(target.c_str()) : net.post(target.c_str());
                                     });
                in_flight = true;
                have_result = false;
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Response");
            ImGui::Separator();

            if (have_result)
            {
                ImVec4 tint = last.status == avNet::response_status::Ok
                                  ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
                                  : ImVec4(0.90f, 0.45f, 0.45f, 1.0f);
                ImGui::TextColored(tint, "Status: %s", status_text(last.status));

                if (last.http_code != 0)
                {
                    ImGui::SameLine();
                    ImGui::Text("(HTTP %ld)", last.http_code);
                }
                if (!last.saved_path.empty())
                {
                    ImGui::TextWrapped("Saved to: %s", last.saved_path.c_str());
                }
            }
            else if (!in_flight)
            {
                ImGui::TextDisabled("No request sent yet.");
            }

            // Scrollable body view fills the rest of the window. ImVec2(0, 0)
            // means "take all remaining content region", so it grows/shrinks with
            // the window; the horizontal scrollbar keeps long lines from clipping.
            ImGui::BeginChild("body", ImVec2(0, 0), ImGuiChildFlags_Border,
                              ImGuiWindowFlags_HorizontalScrollbar);
            if (have_result && !last.body.empty())
            {
                ImGui::TextUnformatted(last.body.c_str(),
                                       last.body.c_str() + last.body.size());
            }
            ImGui::EndChild();

            ImGui::End();

            // Render.
            ImGui::Render();
            int display_w = 0, display_h = 0;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        // If a request is still running, let it finish so the worker isn't left
        // dangling against a destroyed NetworkManager.
        if (in_flight && pending.valid())
        {
            pending.wait();
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

}
