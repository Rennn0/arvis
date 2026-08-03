#include <av_ui/settings_view_ui.hpp>
#include <av_root/ui_scoped_id.hpp>
#include <cfloat>
namespace avUi
{
    constexpr float kNavWidth = 180.f;
    constexpr float kEnvNameWidth = 240.f;
    constexpr float kVarIndent = 12.f;
    constexpr float kWindowW = 840.f;
    constexpr float kWindowH = 560.f;
    constexpr size_t kNoIndex = static_cast<size_t>(-1); // "nothing to erase" sentinel

    const ImVec4 kNavSelectedBg = ImVec4(0.082f, 0.090f, 0.106f, 1.f); // #15171b
    const ImVec4 kNavIdleText = ImVec4(0.322f, 0.337f, 0.369f, 1.f);   // #52565e
    const ImVec4 kNavActiveText = ImVec4(0.843f, 0.851f, 0.867f, 1.f); // #d7d9dd

    const char *const SettingsViewUi::section_labels[] = {
        "General", "Environment", "Shortcuts", "Appearance", "Network",
    };

    SettingsViewUi::SettingsViewUi(std::string id, avR::AvState *sharedState)
        : UiComponent(id), app_settings(std::make_shared<avR::AvAppSettings>()),
          env_storage(std::make_unique<avS::AvEnvironmentStorage>()),
          shared_state(static_cast<avR::AvInterViewSharedState *>(sharedState)),
          selected_section(static_cast<int>(Section::General))
    {
        this->shared_state->on_show_settings.emplace(
            [s = this->shared_state, selected = &this->selected_section](size_t section)
            {
                s->show_settings_view = !s->show_settings_view;
                *selected = section;
            });
        this->shared_state->app_settings = this->app_settings;
    }

    SettingsViewUi::~SettingsViewUi()
    {
    }

    void SettingsViewUi::render()
    {
        if (!this->shared_state->show_settings_view)
        {
            this->envs_loaded = false;
            return;
        }
        using namespace ImGui;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
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
        using namespace ImGui;
        struct Toggle
        {
            const char *label;
            const char *desc;
            bool *val;
        };

        const Toggle toggles[] = {
            {"save responses to disk", "write every response body under responses.dev/",
             &this->app_settings->save_responses},
            {"restore last request", "reopen the request that was selected on exit",
             &this->app_settings->restore_last_req},
        };

        for (const Toggle &t : toggles)
        {
            avR::UiScopedId id(this);
            Checkbox(t.label, t.val);
            SameLine();
            TextDisabled("%s", t.desc);
        }
    }
    void SettingsViewUi::render_environments()
    {
        using namespace ImGui;
        if (!this->envs_loaded)
            this->reload_environments();

        if (Button("+ env"))
        {
            avR::AvEnvironment newEnv;
            newEnv.name = "new env";
            if (this->environments.size() > 0)
                for (const avR::AvEnvironmentVariable &preV : this->environments.front().vars)
                    newEnv.vars.emplace_back(avR::AvEnvironmentVariable{.key = preV.key});
            this->env_storage->upsert(newEnv);
            this->environments.push_back(std::move(newEnv));
        }

        SameLine();
        Spacing();
        if (this->environments.empty())
        {
            TextDisabled("no envs yet");
            return;
        }

        size_t erase_env = kNoIndex;
        for (size_t i = 0; i < this->environments.size(); i++)
        {
            this->render_env_block(this->environments[i], i, erase_env);
        }

        if (erase_env != kNoIndex)
            this->environments.erase(this->environments.begin() + static_cast<long>(erase_env));

        Spacing();
        TextDisabled("reference variables anywhere with {{name}} - params, headers and cookies");
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
    void SettingsViewUi::render_env_block(avR::AvEnvironment &env, size_t index, size_t &erase_env)
    {
        using namespace ImGui;

        avR::UiScopedId sid(static_cast<int>(index));
        if (index > 0)
        {
            Spacing();
            Separator();
            Spacing();
        }

        const bool active = this->is_active_env(env);
        SetNextItemWidth(kEnvNameWidth);
        {
            avR::UiScopedStyle s;
            if (active)
                s.color(ImGuiCol_Text, this->environment_color);

            InputText("##name", &env.name);
        }

        if (IsItemDeactivatedAfterEdit())
        {
            this->env_storage->upsert(env);
            if (active)
                this->set_active_env(env);
        }

        SameLine();
        BeginDisabled(active);
        if (Button("set active"))
            this->set_active_env(env);
        EndDisabled();

        SameLine();
        if (this->env_pending_del == env.id && env.id != 0)
        {
            if (Button("y"))
            {
                this->env_storage->del(env.id);
                erase_env = index;
                this->env_pending_del = 0;
            }

            SameLine();
            if (Button("n"))
            {
                this->env_pending_del = 0;
            }
        }
        else if (Button("x"))
        {
            this->env_pending_del = env.id;
        }
        SetItemTooltip("delete env");

        Indent(kVarIndent);
        size_t eraseVar = kNoIndex;
        if (!env.vars.empty())
        {
            const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable;

            if (BeginTable("##vars", 3, flags))
            {
                TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch, .3f);
                TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, .7f);
                TableSetupColumn("##del", ImGuiTableColumnFlags_WidthFixed);

                for (size_t v = 0; v < env.vars.size(); ++v)
                {
                    avR::AvEnvironmentVariable &var = env.vars[v];

                    avR::UiScopedId varId(static_cast<int>(v));
                    {
                        avR::UiScopedStyle::Style s;
                        s.frame_border = 0;
                        s.frame_rounding = 0;
                        avR::UiScopedStyle ss(s);
                        TableNextRow();
                        TableSetColumnIndex(0);
                        SetNextItemWidth(-FLT_MIN);
                        InputTextWithHint("##key", "name", &var.key);
                        const bool keyCommited = IsItemDeactivatedAfterEdit();

                        TableSetColumnIndex(1);
                        SetNextItemWidth(-FLT_MIN);
                        InputTextWithHint("##val", "value", &var.value);
                        const bool valCommited = IsItemDeactivatedAfterEdit();

                        if (keyCommited || valCommited)
                        {
                            var.EnvId = env.id;
                            this->env_storage->upsert_var(var);
                            if (this->is_active_env(env))
                                this->set_active_env(env);
                        }
                    }

                    TableSetColumnIndex(2);
                    if (Button("x"))
                        eraseVar = v;
                    SameLine();
                    Spacing();
                }
                EndTable();
            }
        }
        else
        {
            TextDisabled("no variables");
        }

        if (eraseVar != kNoIndex)
        {
            this->env_storage->del_var(env.vars[eraseVar].id);
            env.vars.erase(env.vars.begin() + static_cast<long>(eraseVar));
            if (this->is_active_env(env))
                this->set_active_env(env);
        }

        if (Button("+"))
        {
            avR::AvEnvironmentVariable newVar;
            newVar.EnvId = env.id;
            this->env_storage->upsert_var(newVar);
            env.vars.push_back(std::move(newVar));
        }

        Unindent(kVarIndent);
    }
    void SettingsViewUi::reload_environments()
    {
        this->environments = this->env_storage->select_all();
        this->envs_loaded = true;
        this->env_pending_del = 0;
    }
    void SettingsViewUi::set_active_env(const avR::AvEnvironment &env)
    {
        avR::AvRequestListState *s = this->shared_state->request_list_state;
        if (!s || !s->env)
            return;

        *s->env = env;
        this->shared_state->on_env_change.value()();
    }
    bool SettingsViewUi::is_active_env(const avR::AvEnvironment &env) const
    {
        const avR::AvRequestListState *s = this->shared_state->request_list_state;

        return s && s->env && s->env->id == env.id;
    }
} // namespace avUi
