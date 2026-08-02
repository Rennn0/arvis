#include <av_ui/settings_view_ui.hpp>

namespace avUi
{
    constexpr float kNavWidth = 180.f;
    constexpr float kWindowW = 840.f;
    constexpr float kWindowH = 560.f;

    const ImVec4 kNavSelectedBg = ImVec4(0.082f, 0.090f, 0.106f, 1.f); // #15171b
    const ImVec4 kNavIdleText = ImVec4(0.322f, 0.337f, 0.369f, 1.f);   // #52565e
    const ImVec4 kNavActiveText = ImVec4(0.843f, 0.851f, 0.867f, 1.f); // #d7d9dd

    const char *const SettingsViewUi::section_labels[] = {
        "General", "Environment", "Shortcuts", "Appearance", "Network",
    };

    SettingsViewUi::SettingsViewUi(std::string id, avR::AvState *sharedState)
        : UiComponent(id), app_settings(std::make_shared<avR::AvAppSettings>()),
          shared_state(static_cast<avR::AvInterViewSharedState *>(sharedState)),
          selected_section(static_cast<int>(Section::General))
    {
        this->shared_state->on_show_settings.emplace([s = this->shared_state]()
                                                     { s->show_settings_view = !s->show_settings_view; });
    }

    SettingsViewUi::~SettingsViewUi()
    {
    }

    void SettingsViewUi::render()
    {
        if (!this->shared_state->show_settings_view)
            return;
        using namespace ImGui;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
        SetNextWindowPos(GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(.5, .5));
        SetNextWindowSize(ImVec2(kWindowW, kWindowH), ImGuiCond_FirstUseEver);
        SetNextWindowBgAlpha(1.f);
        if (Begin("settings", &this->shared_state->show_settings_view, flags))
        {
            this->render_nav();
            SameLine();
            this->render_content();
            if ((IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && IsKeyPressed(ImGuiKey_Escape)) ||
                (!IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)))
            {
                this->shared_state->show_settings_view = false;
            }
        }
        End();
    }
    void SettingsViewUi::render_nav()
    {
        using namespace ImGui;
        if (BeginChild("##nav", ImVec2(kNavWidth, 0.f)))
        {
            TextDisabled("SETTINGS");
            Spacing();
            avR::UiScopedStyle navS;
            navS.color(ImGuiCol_Header, kNavSelectedBg)
                .color(ImGuiCol_HeaderActive, kNavSelectedBg)
                .color(ImGuiCol_HeaderHovered, kNavSelectedBg);

            for (size_t i = 0; i < static_cast<size_t>(Section::Count); i++)
            {
                const bool active = i == this->selected_section;
                avR::UiScopedStyle navRowS;
                navRowS.color(ImGuiCol_Text, active ? kNavActiveText : kNavIdleText);
                if (Selectable(this->section_labels[i], active))
                {
                    this->selected_section = static_cast<int>(i);
                }
            }
        }

        EndChild();
    }
    void SettingsViewUi::render_content()
    {
        using namespace ImGui;
        if (BeginChild("##content", ImVec2(0.f, 0.f)))
        {
            TextUnformatted(this->section_labels[this->selected_section]);
            Separator();
            Spacing();

            switch (this->selected_section)
            {
            case static_cast<size_t>(Section::General):
                this->render_general();
                break;
            case static_cast<size_t>(Section::Environment):
                this->render_environments();
                break;
            case static_cast<size_t>(Section::Shortcuts):
                this->render_shortcuts();
                break;
            case static_cast<size_t>(Section::Appearance):
                this->render_appearance();
                break;
            case static_cast<size_t>(Section::Network):
                this->render_network();
                break;
            default:
                break;
            }
        }

        EndChild();
    }
    void SettingsViewUi::render_general()
    {
    }
    void SettingsViewUi::render_environments()
    {
    }
    void SettingsViewUi::render_shortcuts()
    {
    }
    void SettingsViewUi::render_appearance()
    {
    }
    void SettingsViewUi::render_network()
    {
    }
} // namespace avUi
