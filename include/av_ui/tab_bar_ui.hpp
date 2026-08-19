#pragma once
#include <av_root/ui_component.hpp>
#include <vector>

namespace avUi
{
    class TabBarUi : public avR::UiComponent
    {
    public:
        TabBarUi(std::string id, float marginX = 15.f, float trackPad = 3.f);
        ~TabBarUi();

        void addTab(const char *label, bool *badgeEnabled = nullptr, int *countBadge = nullptr);

        const size_t getActiveTab()const;

    private:
        struct Segment
        {
            bool *badgeEnabled;
            const char *label;
            int *countBadge;
        };
        std::vector<Segment> segments;

        bool _tabPillReady;
        int _activeTab;
        float _tabPillOff;
        const float _marginX;
        const float _trackPad;

        void render() override;
    };
} // namespace avUi
