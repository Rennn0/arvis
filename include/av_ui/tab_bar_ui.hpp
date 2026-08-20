#pragma once
#include <av_root/ui_component.hpp>
#include <vector>

namespace avUi
{
    class TabBarUi : public avR::UiComponent
    {
    public:
        enum class Sizing
        {
            stretch,
            fit,
        };

        TabBarUi(std::string id, float marginX = 15.f, float trackPad = 3.f, Sizing sizing = Sizing::stretch,
                 float segMinW = 0.f, float segPadX = 18.f);
        ~TabBarUi();

        void addTab(const char *label, bool *badgeEnabled = nullptr, int *countBadge = nullptr);

        const size_t getActiveTab() const;

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
        float _tabPillW;
        const float _marginX;
        const float _trackPad;
        const Sizing _sizing;
        const float _segMinW; 
        const float _segPadX; 

        /// @brief Writes a segment's badge text into buff; false when the segment has no badge.
        ///        Shared by the width measurement and the draw loop so the two cannot disagree.
        bool format_badge(const Segment &seg, char *buff, size_t buffSize) const;

        /// @brief Width one segment needs to show its label and badge without crowding.
        float measure_segment(const Segment &seg) const;

        void render() override;
    };
} // namespace avUi
