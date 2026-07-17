#include <av_root/av_div.hpp>
#include <av_root/im_scope.hpp>

#include <algorithm>
#include <cstddef>

namespace avR
{
    AvDiv::AvDiv(std::string id, Config config)
        : UiContainerComponent(std::move(id)), config(config)
    {
    }

    AvDiv &AvDiv::configure(const Config &config)
    {
        this->config = config;
        return *this;
    }

    void AvDiv::set_layout_size(const ImVec2 &size)
    {
        this->config.layoutSize = size;
        this->config.hasLayoutSize = true;
    }

    ImVec2 AvDiv::preferred_size() const
    {
        return scale_size(this->config.size);
    }

    void AvDiv::render()
    {
        // Apply configurable spacing/padding for the region's scope. The guard
        // unwinds every push at end of scope — no manual pop bookkeeping.
        ScopedStyle style;
        const float spacing = scale_px(this->config.spacing);
        style.var(ImGuiStyleVar_WindowPadding, scale_size(this->config.padding))
            .var(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

        if (this->config.background.w > 0.0f)
            style.color(ImGuiCol_ChildBg, this->config.background);

        // Without Borders, BeginChild ignores WindowPadding unless this flag is set.
        ImGuiChildFlags childFlags = ImGuiChildFlags_AlwaysUseWindowPadding;
        if (this->config.border)
            childFlags |= ImGuiChildFlags_Borders;

        // A resizable parent may have imposed a size on us this frame; otherwise
        // fall back to the configured size ((0,0) => fill available space).
        // Imposed sizes are already in screen px; config sizes are design units.
        const ImVec2 size = this->config.hasLayoutSize ? this->config.layoutSize : scale_size(this->config.size);

        if (ImGui::BeginChild(get_id().c_str(), size, childFlags))
        {
            layout_children();
        }
        ImGui::EndChild();

        // The imposed size lasts a single frame; the parent re-applies it each draw.
        this->config.hasLayoutSize = false;
    }

    void AvDiv::layout_children()
    {
        const auto &kids = get_children();
        if (kids.empty())
            return;

        const bool horizontal = (this->config.direction == Direction::Horizontal);
        const float s = ui_scale();

        // --- simple (non-resizable) flow ------------------------------------
        if (!this->config.resizable)
        {
            if (horizontal)
            {
                // Vertically center the row within the padded content area.
                float rowH = 0.0f;
                for (const std::unique_ptr<UiComponent> &child : kids)
                {
                    const float h = child->preferred_size().y;
                    rowH = std::max(rowH, h > 0.0f ? h : ImGui::GetFrameHeight());
                }
                const float availH = ImGui::GetContentRegionAvail().y;
                if (availH > rowH)
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availH - rowH) * 0.5f);
            }

            bool first = true;
            for (const std::unique_ptr<UiComponent> &child : kids)
            {
                if (horizontal && !first)
                    ImGui::SameLine();
                child->draw();
                first = false;
            }
            return;
        }

        // --- resizable: each leading child gets a stored main-axis extent plus
        //     a draggable splitter on its trailing edge; the last child fills.
        //     Seed extents once from each child's preferred size.
        if (this->config.extents.size() != kids.size())
        {
            this->config.extents.assign(kids.size(), 0.0f);
            for (std::size_t i = 0; i < kids.size(); ++i)
            {
                const ImVec2 pref = kids[i]->preferred_size();
                this->config.extents[i] = horizontal ? pref.x : pref.y;
            }
            this->config.extentsScale = s;
        }
        else if (this->config.extentsScale > 0.0f && this->config.extentsScale != s)
        {
            const float ratio = s / this->config.extentsScale;
            for (float &extent : this->config.extents)
                extent *= ratio;
            this->config.extentsScale = s;
        }

        bool first = true;
        const auto gap = [&]()
        {
            if (horizontal && !first)
                ImGui::SameLine();
            first = false;
        };

        for (std::size_t i = 0; i < kids.size(); ++i)
        {
            const bool last = (i + 1 == kids.size());

            gap();
            if (last)
            {
                kids[i]->set_layout_size(ImVec2(0.0f, 0.0f)); // fill remaining
                kids[i]->draw();
                continue;
            }

            // Leading child: pin its main-axis extent, let the cross axis fill.
            kids[i]->set_layout_size(horizontal ? ImVec2(this->config.extents[i], 0.0f)
                                                : ImVec2(0.0f, this->config.extents[i]));
            kids[i]->draw();

            // Draggable splitter on the trailing edge.
            gap();
            ScopedId splitterId(static_cast<int>(i));

            const ImVec2 region = ImGui::GetContentRegionAvail();
            const float thickness = scale_px(this->config.splitter_thickness);
            const ImVec2 handle = horizontal
                                      ? ImVec2(thickness, region.y)
                                      : ImVec2(region.x, thickness);

            {
                ScopedStyle splitterStyle;
                splitterStyle.color(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.06f))
                    .color(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.20f))
                    .color(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
                ImGui::Button("##splitter", handle);
            }

            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                ImGui::SetMouseCursor(horizontal ? ImGuiMouseCursor_ResizeEW
                                                 : ImGuiMouseCursor_ResizeNS);

            if (ImGui::IsItemActive())
                this->config.extents[i] += horizontal ? ImGui::GetIO().MouseDelta.x
                                           : ImGui::GetIO().MouseDelta.y;

            // Clamp to the configured min/max (design units → screen px).
            const float resizeMin = scale_px(this->config.resize_min);
            const float resizeMax = scale_px(this->config.resize_max);
            if (this->config.extents[i] < resizeMin)
                this->config.extents[i] = resizeMin;
            if (resizeMax > 0.0f && this->config.extents[i] > resizeMax)
                this->config.extents[i] = resizeMax;
        }
    }
} // namespace avR
