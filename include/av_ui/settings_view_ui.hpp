#pragma once
#include <av_root/ui_component.hpp>
#include <av_root/av_app_settings.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

namespace avUi
{
    class SettingsViewUi : public avR::UiComponent
    {
    public:
        explicit SettingsViewUi(std::string id, avR::AvState *sharedState);
        ~SettingsViewUi();

    private:
        std::shared_ptr<avR::AvAppSettings> app_settings;
        avR::AvInterViewSharedState *shared_state;

        enum class Section
        {
            General = 0,
            Environment,
            Shortcuts,
            Appearance,
            Network,
            Count
        };

        static const char *const section_labels[static_cast<int>(Section::Count)];

        size_t selected_section;
        void render() override;

        void render_nav();
        void render_content();

        void render_general();
        void render_environments();
        void render_shortcuts();
        void render_appearance();
        void render_network();
    };
} // namespace avUi
