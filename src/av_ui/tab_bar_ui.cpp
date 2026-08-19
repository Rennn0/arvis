#include <av_ui/tab_bar_ui.hpp>
#include <av_root/ui_scoped_id.hpp>
#include <cmath>

namespace avUi
{
    TabBarUi::TabBarUi(std::string id, float marginX, float trackPad)
        : UiComponent(id), _tabPillReady(false), _activeTab(-1), _tabPillOff(0), _marginX(marginX), _trackPad(trackPad)
    {
    }

    TabBarUi::~TabBarUi()
    {
    }

    void TabBarUi::addTab(const char *label, bool *badgeEnabled, int *countBadge)
    {
        this->segments.emplace_back(Segment{.badgeEnabled = badgeEnabled, .label = label, .countBadge = countBadge});

        if (this->_activeTab < 0)
            this->_activeTab = 0;
    }

    const size_t TabBarUi::getActiveTab() const
    {
        return static_cast<size_t>(this->_activeTab);
    }

    void TabBarUi::render()
    {
        using namespace ImGui;
        const size_t segmentCount = this->segments.size();
        const ImVec2 origin = GetCursorPos();
        const float trackW = GetContentRegionAvail().x - this->_marginX * 2;
        const float trackH = GetFrameHeight() + this->_trackPad * 2;

        SetCursorPos(ImVec2(origin.x + this->_marginX, origin.y + this->_trackPad));
        const ImVec2 trackMin = GetCursorScreenPos();
        const ImVec2 trackMax = ImVec2(trackMin.x + trackW, trackMin.y + trackH);
        const float segW = (trackW - this->_trackPad * 2) / segmentCount;
        const float segH = trackH - this->_trackPad * 2;

        int hovered = -1;
        for (size_t i = 0; i < segmentCount; i++)
        {
            avR::UiScopedId id(i);
            SetCursorScreenPos(ImVec2(trackMin.x + this->_trackPad + segW * i, trackMin.y + this->_trackPad));
            if (InvisibleButton("##seg", ImVec2(segW, segH)))
                this->_activeTab = i;
            if (IsItemHovered())
            {
                hovered = i;
                SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }

        const float targetOff = segW * this->_activeTab;
        if (!this->_tabPillReady)
        {
            this->_tabPillOff = targetOff;
            this->_tabPillReady = true;
        }
        else
        {
            const float dt = std::clamp(GetIO().DeltaTime, 0.f, 1.f / 60.f);
            this->_tabPillOff += (targetOff - this->_tabPillOff) * std::clamp(dt * 18.f, 0.f, 1.f);
            if (std::fabs(targetOff - this->_tabPillOff) <= .5)
                this->_tabPillOff = targetOff;
        }

        ImDrawList *dl = GetWindowDrawList();
        dl->AddRectFilled(trackMin, trackMax, IM_COL32(255, 255, 255, 12), trackH * .5);
        dl->AddRect(trackMin, trackMax, IM_COL32(255, 255, 255, 18), trackH * .5);

        if (hovered >= 0 && hovered != this->_activeTab)
        {
            const ImVec2 hoverMin = ImVec2(trackMin.x + this->_trackPad + segW * hovered, trackMin.y + this->_trackPad);
            dl->AddRectFilled(hoverMin, ImVec2(hoverMin.x + segW, hoverMin.y + segH), IM_COL32(255, 255, 255, 16),
                              segH * .5);
        }

        ImVec4 accent = GetStyleColorVec4(ImGuiCol_TabSelected);
        accent.w = .95;
        const ImVec2 pillMin = ImVec2(trackMin.x + this->_trackPad + this->_tabPillOff, trackMin.y + this->_trackPad);
        const ImVec2 pillMax = ImVec2(pillMin.x + segW, pillMin.y + segH);
        dl->AddRectFilled(pillMin, pillMax, GetColorU32(accent), segH * .5);
        dl->AddRect(pillMin, pillMax, IM_COL32(255, 255, 255, 30), segH * .5);

        for (size_t i = 0; i < segmentCount; i++)
        {
            const Segment &seg = this->segments[i];
            const bool active = i == this->_activeTab;
            const bool hasBadge = seg.badgeEnabled && *seg.badgeEnabled && seg.countBadge;
            char badgeBuff[8];
            if (hasBadge && *seg.countBadge > 99)
                std::snprintf(badgeBuff, sizeof(badgeBuff), "99+");
            else if (hasBadge)
                std::snprintf(badgeBuff, sizeof(badgeBuff), "%d", *seg.countBadge);

            const ImVec2 labelSize = CalcTextSize(seg.label);
            const ImVec2 badgeSize = hasBadge ? CalcTextSize(badgeBuff) : ImVec2(0, 0);
            const float badgeH = badgeSize.y + 5;
            const float badgeW = hasBadge ? std::max(badgeSize.x + 10, badgeH) : 0;
            const float gap = hasBadge ? 6 : 0;

            const float segX = trackMin.x + this->_trackPad + segW * i;
            const float labelX = segX + (segW - (labelSize.x + gap + badgeW)) * .5;
            const float labelY = trackMin.y + this->_trackPad + (segH - labelSize.y) * .5;

            ImU32 labelColor;
            if (active)
                labelColor = IM_COL32_WHITE;
            else if (i == hovered)
                labelColor = GetColorU32(ImGuiCol_Text);
            else
                labelColor = GetColorU32(ImGuiCol_TextDisabled);

            dl->AddText(ImVec2(labelX, labelY), labelColor, seg.label);

            if (!hasBadge)
                continue;

            const ImVec2 badgeMin =
                ImVec2(labelX + labelSize.x + gap, trackMin.y + this->_trackPad + (segH - badgeH) * .5);
            dl->AddRectFilled(badgeMin, ImVec2(badgeMin.x + badgeW, badgeMin.y + badgeH),
                              active ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 20), badgeH * .5);
            dl->AddText(ImVec2(badgeMin.x + (badgeW - badgeSize.x) * .5, badgeMin.y + (badgeH - badgeSize.y) * .5),
                        active ? IM_COL32_WHITE : GetColorU32(ImGuiCol_TextDisabled), badgeBuff);
        }

        SetCursorPos(ImVec2(origin.x, origin.y + trackH + this->_trackPad));
    }
} // namespace avUi
