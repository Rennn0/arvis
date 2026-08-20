#include <av_ui/tab_bar_ui.hpp>
#include <av_root/ui_scoped_id.hpp>
#include <boost/container/small_vector.hpp>
#include <algorithm>
#include <cmath>

namespace avUi
{
    TabBarUi::TabBarUi(std::string id, float marginX, float trackPad, Sizing sizing, float segMinW, float segPadX)
        : UiComponent(id), _tabPillReady(false), _activeTab(-1), _tabPillOff(0), _tabPillW(0), _marginX(marginX),
          _trackPad(trackPad), _sizing(sizing), _segMinW(segMinW), _segPadX(segPadX)
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
        const size_t at = static_cast<size_t>(this->_activeTab);
        return at;
    }

    bool TabBarUi::format_badge(const Segment &seg, char *buff, size_t buffSize) const
    {
        if (!seg.badgeEnabled || !*seg.badgeEnabled || !seg.countBadge)
            return false;

        if (*seg.countBadge > 99)
            std::snprintf(buff, buffSize, "99+");
        else
            std::snprintf(buff, buffSize, "%d", *seg.countBadge);

        return true;
    }

    float TabBarUi::measure_segment(const Segment &seg) const
    {
        char badgeBuff[8];
        const bool hasBadge = this->format_badge(seg, badgeBuff, sizeof(badgeBuff));

        float w = ImGui::CalcTextSize(seg.label).x + this->_segPadX * 2;
        if (hasBadge)
        {
            // same numbers the draw loop below uses, or the label would not sit centered
            const ImVec2 badgeSize = ImGui::CalcTextSize(badgeBuff);
            const float badgeH = badgeSize.y + 5;
            w += 6 + std::max(badgeSize.x + 10, badgeH); // gap + badge pill
        }

        return std::max(w, this->_segMinW);
    }

    void TabBarUi::render()
    {
        using namespace ImGui;
        const size_t segmentCount = this->segments.size();
        if (segmentCount == 0)
            return; // nothing to slice, and the equal split below would divide by zero

        const ImVec2 origin = GetCursorPos();
        const float availW = GetContentRegionAvail().x - this->_marginX * 2;
        const float trackH = GetFrameHeight() + this->_trackPad * 2;
        const float segH = trackH - this->_trackPad * 2;

        // --- per-segment widths --------------------------------------------------------
        // a tab bar never has eight tabs, so small_vector keeps this off the heap.
        boost::container::small_vector<float, 8> segW;
        float contentW = 0;
        if (this->_sizing == Sizing::fit)
        {
            for (const Segment &seg : this->segments)
            {
                const float w = this->measure_segment(seg);
                segW.push_back(w);
                contentW += w;
            }
        }

        // stretch mode, or a fit that does not actually fit: equal slices of the region. The
        // fallback matters - without it a narrow panel would draw the track past its own edge.
        if (this->_sizing != Sizing::fit || contentW + this->_trackPad * 2 > availW)
        {
            contentW = availW - this->_trackPad * 2;
            segW.assign(segmentCount, contentW / segmentCount);
        }

        const float trackW = contentW + this->_trackPad * 2;

        // x offset of every segment inside the track: prefix sum of the widths
        boost::container::small_vector<float, 8> segOff;
        segOff.reserve(segmentCount);
        float run = 0;
        for (const float w : segW)
        {
            segOff.push_back(run);
            run += w;
        }

        SetCursorPos(ImVec2(origin.x + this->_marginX, origin.y + this->_trackPad));
        const ImVec2 trackMin = GetCursorScreenPos();
        const ImVec2 trackMax = ImVec2(trackMin.x + trackW, trackMin.y + trackH);

        int hovered = -1;
        for (size_t i = 0; i < segmentCount; i++)
        {
            avR::UiScopedId id(static_cast<int>(i));
            SetCursorScreenPos(ImVec2(trackMin.x + this->_trackPad + segOff[i], trackMin.y + this->_trackPad));
            if (InvisibleButton("##seg", ImVec2(segW[i], segH)))
                this->_activeTab = static_cast<int>(i);
            if (IsItemHovered())
            {
                hovered = static_cast<int>(i);
                SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }

        // --- pill: offset and width both ease toward the active segment ----------------
        const size_t active = static_cast<size_t>(std::max(this->_activeTab, 0));
        const float targetOff = segOff[active];
        const float targetW = segW[active];
        if (!this->_tabPillReady)
        {
            this->_tabPillOff = targetOff;
            this->_tabPillW = targetW;
            this->_tabPillReady = true;
        }
        else
        {
            const float dt = std::clamp(GetIO().DeltaTime, 0.f, 1.f / 60.f);
            const float t = std::clamp(dt * 18.f, 0.f, 1.f);
            this->_tabPillOff += (targetOff - this->_tabPillOff) * t;
            this->_tabPillW += (targetW - this->_tabPillW) * t;
            if (std::fabs(targetOff - this->_tabPillOff) <= .5)
                this->_tabPillOff = targetOff;
            if (std::fabs(targetW - this->_tabPillW) <= .5)
                this->_tabPillW = targetW;
        }

        ImDrawList *dl = GetWindowDrawList();
        dl->AddRectFilled(trackMin, trackMax, IM_COL32(72, 72, 77, 25), trackH * .5);
        dl->AddRect(trackMin, trackMax, IM_COL32(72, 72, 77, 55), trackH * .5);
        if (hovered >= 0 && hovered != this->_activeTab)
        {
            const ImVec2 hoverMin =
                ImVec2(trackMin.x + this->_trackPad + segOff[hovered], trackMin.y + this->_trackPad);
            dl->AddRectFilled(hoverMin, ImVec2(hoverMin.x + segW[hovered], hoverMin.y + segH),
                              IM_COL32(255, 255, 255, 16), segH * .5);
        }

        ImVec4 accent = GetStyleColorVec4(ImGuiCol_TabSelected);
        const ImVec2 pillMin = ImVec2(trackMin.x + this->_trackPad + this->_tabPillOff, trackMin.y + this->_trackPad);
        const ImVec2 pillMax = ImVec2(pillMin.x + this->_tabPillW, pillMin.y + segH);
        dl->AddRectFilled(pillMin, pillMax, GetColorU32(accent), segH * .5);
        dl->AddRect(pillMin, pillMax, IM_COL32(255, 255, 255, 30), segH * .5);

        for (size_t i = 0; i < segmentCount; i++)
        {
            const Segment &seg = this->segments[i];
            const bool active = i == static_cast<size_t>(this->_activeTab);
            char badgeBuff[8];
            const bool hasBadge = this->format_badge(seg, badgeBuff, sizeof(badgeBuff));

            const ImVec2 labelSize = CalcTextSize(seg.label);
            const ImVec2 badgeSize = hasBadge ? CalcTextSize(badgeBuff) : ImVec2(0, 0);
            const float badgeH = badgeSize.y + 5;
            const float badgeW = hasBadge ? std::max(badgeSize.x + 10, badgeH) : 0;
            const float gap = hasBadge ? 6 : 0;

            const float segX = trackMin.x + this->_trackPad + segOff[i];
            const float labelX = segX + (segW[i] - (labelSize.x + gap + badgeW)) * .5;
            const float labelY = trackMin.y + this->_trackPad + (segH - labelSize.y) * .5;

            ImU32 labelColor;
            if (active)
                labelColor = IM_COL32_WHITE;
            else if (i == static_cast<size_t>(hovered))
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
