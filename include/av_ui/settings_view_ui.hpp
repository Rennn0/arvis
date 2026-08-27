#pragma once
#include <av_root/ui_component.hpp>
#include <av_root/av_app_settings.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_s/av_environment_storage.hpp>
#include <av_s/av_app_settings_storage.hpp>
#include <av_ui/icons_font_awesome7.hpp>
#include <boost/container/small_vector.hpp>

namespace avUi
{
    enum class Section
    {
        General = 0,
        Environment,
        Shortcuts,
        Appearance,
        Network,
        Count
    };

    class SettingsViewUi : public avR::UiComponent
    {
    public:
        explicit SettingsViewUi(std::string id, avR::AvState *sharedState);
        ~SettingsViewUi();

    private:
        struct Toggle
        {
            const char *label;
            const char *desc;
            bool *val;
        };

        struct SettingsToggle
        {
            const uint8_t id;
            const char *label;
            const ImU32 color;
            std::function<void()> action;
        };

        std::shared_ptr<avR::AvAppSettings> _app_settings;
        std::unique_ptr<avS::AvEnvironmentStorage> env_storage;
        std::unique_ptr<avS::AvAppSettingsStorage> _app_settings_storage;
        avR::AvInterViewSharedState *shared_state;
        std::vector<avR::AvEnvironment> environments;
        bool envs_loaded = false;
        bool _run_update = true;
        int64_t env_pending_del = 0;

        static const char *const section_labels[static_cast<int>(Section::Count)];

        size_t selected_section;

        void render() override;
        void update() override;

        void apply_settings(const avR::AvAppSettings &settings);
        void render_nav();
        void render_content();

        void render_general();
        void render_environments();
        void render_shortcuts();
        void render_appearance();
        void render_appearance_theme();
        void render_appearance_fonts();
        void render_network();

        void render_env_block(avR::AvEnvironment &env, size_t index, size_t &erase_env);
        void reload_environments();
        void set_active_env(const avR::AvEnvironment &env);
        bool is_active_env(const avR::AvEnvironment &env) const;

        const SettingsToggle _theme_settings[3] = {SettingsToggle{.id = 0,
                                                                  .label = ICON_FA_SUN " Light",
                                                                  .color = this->lightThemeColor,
                                                                  .action =
                                                                      [this]()
                                                                  {
                                                                      this->_app_settings->_active_theme_id = 0;
                                                                      ImGui::StyleColorsLight();
                                                                  }},
                                                   SettingsToggle{.id = 1,
                                                                  .label = ICON_FA_MOON " Dark",
                                                                  .color = this->darkThemeColor,
                                                                  .action =
                                                                      [this]()
                                                                  {
                                                                      this->_app_settings->_active_theme_id = 1;
                                                                      ImGui::StyleColorsDark();
                                                                  }},
                                                   SettingsToggle{.id = 2,
                                                                  .label = ICON_FA_CIRCLE_HALF_STROKE " Classic",
                                                                  .color = this->classicThemeColor,
                                                                  .action = [this]()
                                                                  {
                                                                      this->_app_settings->_active_theme_id = 2;
                                                                      ImGui::StyleColorsClassic();
                                                                  }}};

        boost::container::small_vector<SettingsToggle, 8> _font_settings;
    };
} // namespace avUi
