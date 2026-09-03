#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/settings_view_ui.hpp>
#include <av_ui/logo_icon.hpp>
#include <av_ui/icons_font_awesome7.hpp>
#include <av_s/av_environment_storage.hpp>
#include <av_root/ui_scoped_id.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/small_vector.hpp>
#include <ranges>
#include <algorithm>
#include <chrono>
#include <cstdio>

namespace avUi
{
    bool try_swap(std::vector<std::shared_ptr<avR::AvRequest>> &vec, avR::AvRequest *r1, avR::AvRequest *r2)
    {
        auto it1 = std::find_if(vec.begin(), vec.end(),
                                [r1](const std::shared_ptr<avR::AvRequest> &p) { return p.get() == r1; });
        auto it2 = std::find_if(vec.begin(), vec.end(),
                                [r2](const std::shared_ptr<avR::AvRequest> &p) { return p.get() == r2; });

        if (it1 != vec.end() && it2 != vec.end())
        {
            std::iter_swap(it1, it2);
            return true;
        }

        return false;
    }

    void apply_reordering(std::vector<std::shared_ptr<avR::AvRequest>> &vec)
    {
        for (size_t i = 0; i < vec.size(); i++)
        {
            vec[i]->order_by = i;
        }
    }

    void load_env(avR::AvEnvironment *env)
    {
        avS::AvEnvironmentStorage es;
        std::vector<avR::AvEnvironment> envs = es.select_all();
        if (envs.empty())
            return;

        *env = std::move(envs.front());
    }

    /// @brief "1.2 KB" / "340 B" / "3.1 MB" - one short token for a body size.
    static std::string format_bytes(size_t n)
    {
        char buf[32];
        if (n < 1024)
            std::snprintf(buf, sizeof(buf), "%zu B", n);
        else if (n < 1024ull * 1024ull)
            std::snprintf(buf, sizeof(buf), "%.1f KB", static_cast<double>(n) / 1024.0);
        else
            std::snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(n) / (1024.0 * 1024.0));
        return buf;
    }

    /// @brief "YYYY-MM-DD HH:MM:SS" (AvRoot::timestamp_to_date) -> "HH:MM:SS".
    ///        History rows are grouped per request, so the date is redundant on the row and
    ///        only shown in the tooltip.
    static std::string time_of_day(const std::string &full_ts)
    {
        return full_ts.size() >= 19 ? full_ts.substr(11, 8) : full_ts;
    }

    RequstListViewUi::RequstListViewUi(std::string id)
        : avR::UiComponent(std::move(id)), request_list_state(std::make_shared<avR::AvRequestListState>()),
          request_storage(std::make_unique<avS::AvRequestStorage>())
    {
        this->windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    RequstListViewUi::RequstListViewUi(std::string id, avR::AvState *sharedState) : RequstListViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
        this->shared_state->request_list_state = this->request_list_state.get();
        this->request_list_state->requests = this->request_storage->select_all();
        this->request_list_state->env = std::make_shared<avR::AvEnvironment>();
        load_env(this->request_list_state->env.get());

        if (this->request_list_state->requests.size() > 0)
        {
            const auto &latest =
                std::ranges::max_element(this->request_list_state->requests, {}, &avR::AvRequest::timestamp);
            this->shared_state->display_request = latest->get();
            this->shared_state->display_request_hold.reset();
        }

        this->shared_state->on_new_request.emplace([this]() { this->new_request(); });

        this->tabs = std::make_unique<avUi::TabBarUi>("tabs");
        this->_tab_badge_enabled = true;
        this->tabs->addTab(ICON_FA_CLOCK "  history", &this->_tab_badge_enabled, &this->_tab_history_counter_badge);
        this->tabs->addTab(ICON_FA_FLOPPY_DISK "  saved", &this->_tab_badge_enabled, &this->_tab_saved_counter_badge);
    }

    RequstListViewUi::~RequstListViewUi()
    {
    }

    void RequstListViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_list_view)
            return;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const float x = viewport->WorkSize.x;
        const float y = viewport->WorkSize.y;
        const float panelW =
            std::max(this->shared_state->_min_left_panel_width, x * this->shared_state->_left_panel_ratio);
        ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelW, y), ImGuiCond_Always);

        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_list_view, this->windowFlags))
        {
            const ImGuiStyle &style = ImGui::GetStyle();
            const ImVec2 avail_region = ImGui::GetContentRegionAvail();
            const float footer_height = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            const float header_height = (avail_region.y - footer_height) * .1f;

            ImGui::BeginChild("header", ImVec2(0, header_height));
            this->render_header(style);
            ImGui::EndChild();

            ImGui::BeginChild("main_content", ImVec2(0, -footer_height));
            this->render_main_content(style);
            ImGui::EndChild();

            // Fixed footer at bottom
            ImGui::Separator();
            ImGui::BeginChild("footer", ImVec2(0, 0));
            this->render_footer(style);
            ImGui::EndChild();
        };
        ImGui::End();
    }

    void RequstListViewUi::update()
    {
        // derived, not counted: a running counter goes stale the moment a snapshot is
        // removed or its request is deleted.
        size_t recent = 0;
        for (const std::shared_ptr<avR::AvRequest> &req : this->request_list_state->requests)
            recent += req->recent_reqs.size();

        this->_tab_history_counter_badge = static_cast<int>(recent);
        this->_tab_saved_counter_badge = static_cast<int>(this->request_list_state->requests.size());
    }

    void RequstListViewUi::render_header(const ImGuiStyle &style)
    {
        const float marginTop = 10.f;
        const float marginLeft = 15.f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + marginTop);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + marginLeft);

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled(ICON_FA_LAYER_GROUP "  env:");
        ImGui::SameLine();
        const std::string &envName = this->request_list_state->env->name;
        if (envName.empty())
            ImGui::TextDisabled("none");
        else
            ImGui::TextColored(this->environment_color, "%s", envName.c_str());

        const char *addLabel = ICON_FA_CIRCLE_PLUS;
        const float addLabelButtonWidth = ImGui::CalcTextSize(addLabel).x + style.FramePadding.x * 3.f;
        const char *settingsLabel = ICON_FA_GEAR;
        const float settingsLabelWidth = ImGui::CalcTextSize(addLabel).x + style.FramePadding.x * 3.f;
        const float marginRight = addLabelButtonWidth + settingsLabelWidth;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - marginRight);
        if (ImGui::Button(addLabel))
            this->new_request();
        ImGui::SetItemTooltip("add request");

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - marginRight +
                             addLabelButtonWidth);
        if (ImGui::Button(settingsLabel))
            this->shared_state->on_show_settings.value()(static_cast<size_t>(avUi::Section::General));
        ImGui::SetItemTooltip("open settings");

        ImGui::Dummy(ImVec2(0.f, marginTop));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + marginLeft);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - marginLeft);
        ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS "  filter requsts", &this->filter_text);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            this->log_info(this->filter_text);
        }
    }
    void RequstListViewUi::render_main_content(const ImGuiStyle &style)
    {
        this->tabs->draw();
        switch (this->tabs->getActiveTab())
        {
        case 0:
            this->render_tab_history(style);
            break;
        case 1:
            this->render_tab_saved(style);
            break;
        default:
            this->render_tab_history(style);
            break;
        }

        // drained here, not inside a tab: a delete issued from the history tab used to sit
        // pending until the user happened to open the saved tab.
        this->apply_history_action();
        this->apply_pending_delete();
    }
    void RequstListViewUi::render_footer(const ImGuiStyle &style)
    {
        const float savedTxtOffset = 15.f;
        const int savedCount = this->request_list_state->requests.size();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + savedTxtOffset);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(ICON_FA_FLOPPY_DISK "  %d saved", savedCount);

        const char *envLabel = ICON_FA_LAYER_GROUP "  env";
        const float envLabelButtonWidth = ImGui::CalcTextSize(envLabel).x + style.FramePadding.x * 3.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - envLabelButtonWidth);

        if (ImGui::Button(envLabel))
        {
            this->shared_state->on_show_settings.value()(static_cast<size_t>(avUi::Section::Environment));
        }
        ImGui::SetItemTooltip("modify environment variables");
    }

    void RequstListViewUi::render_tab_history(const ImGuiStyle &imstyle)
    {
        // Groups ordered by their most recent send, newest first - the useful reading order
        // for a history log.
        boost::container::small_vector<avR::AvRequest *, 16> groups;
        for (const std::shared_ptr<avR::AvRequest> &req : this->request_list_state->requests)
        {
            if (!req->recent_reqs.empty())
                groups.push_back(req.get());
        }
        std::ranges::sort(groups, [](const avR::AvRequest *a, const avR::AvRequest *b)
                          { return a->recent_reqs.back()->timestamp > b->recent_reqs.back()->timestamp; });

        if (groups.empty())
        {
            const char *msg = ICON_FA_CLOCK_ROTATE_LEFT "  No history yet";
            const char *hint = "sent requests are recorded here";
            const float avail_w = ImGui::GetContentRegionAvail().x;
            const float avail_h = ImGui::GetContentRegionAvail().y;

            ImGui::Dummy(ImVec2(0.f, std::max(avail_h * .3f, 20.f)));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_w - ImGui::CalcTextSize(msg).x) * .5f);
            ImGui::TextDisabled("%s", msg);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail_w - ImGui::CalcTextSize(hint).x) * .5f);
            ImGui::TextDisabled("%s", hint);
            return;
        }

        ImGui::Dummy(ImVec2(0.f, 4.f));
        for (avR::AvRequest *origin : groups)
            this->render_history_group(origin, imstyle);
    }

    void RequstListViewUi::render_history_group(avR::AvRequest *origin, const ImGuiStyle &imstyle)
    {
        avR::UiScopedId scid(origin);

        // display_name(), not title.value(): a request created with ctrl+n has no title and
        // .value() throws std::bad_optional_access.
        const std::string name = origin->display_name();
        const bool name_hit = this->filter_text.empty() || boost::icontains(name, this->filter_text);

        // Rows for this group, newest first. shared_ptr copies (not raw pointers into
        // recent_reqs) so a "remove"/"clear" requested from a row's popup cannot invalidate
        // what the loop is walking, and the row keeps something to hand to select_snapshot.
        boost::container::small_vector<std::shared_ptr<avR::AvRequest>, 16> rows;
        for (auto it = origin->recent_reqs.rbegin(); it != origin->recent_reqs.rend(); ++it)
        {
            if (name_hit || boost::icontains((*it)->url, this->filter_text))
                rows.push_back(*it);
        }
        if (rows.empty())
            return;

        const ImVec4 accent = this->get_method_color(origin->method);
        const ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_DefaultOpen;

        // --- group header: method-tinted pill with a coloured accent bar -------------
        bool open = false;
        {
            avR::UiScopedStyle head;
            head.var(ImGuiStyleVar_FramePadding, ImVec2(imstyle.FramePadding.x, 5.f))
                .color(ImGuiCol_Header, ImVec4(accent.x, accent.y, accent.z, 0.10f))
                .color(ImGuiCol_HeaderHovered, ImVec4(accent.x, accent.y, accent.z, 0.20f))
                .color(ImGuiCol_HeaderActive, ImVec4(accent.x, accent.y, accent.z, 0.28f))
                .color(ImGuiCol_Text, accent);

            ImGui::SetNextItemAllowOverlap(); // the clear button sits on top of the spanned node
            open = ImGui::TreeNodeEx("##group", flags, "%s  %s",
                                     avNet::NetworkManager::method_text(origin->method), name.c_str());
        }

        const ImVec2 head_min = ImGui::GetItemRectMin();
        const ImVec2 head_max = ImGui::GetItemRectMax();
        const float head_mid_y = (head_min.y + head_max.y) * .5f;
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(head_min.x, head_min.y + 2.f), ImVec2(head_min.x + 3.f, head_max.y - 2.f),
                          ImGui::GetColorU32(accent), 1.5f);

        // clear-history button, right-aligned on the header line
        const char *clear_label = ICON_FA_BROOM;
        const float clear_w = ImGui::CalcTextSize(clear_label).x + imstyle.FramePadding.x * 2.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - clear_w);
        {
            avR::UiScopedStyle btn;
            btn.color(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f))
                .color(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            if (ImGui::SmallButton(clear_label))
                this->pending_history_action = {HistoryAction::Kind::clear, origin->id, 0};
        }
        ImGui::SetItemTooltip("clear history for this request");

        // snapshot-count pill, just left of the clear button
        {
            char cnt[16];
            std::snprintf(cnt, sizeof(cnt), "%zu", rows.size());
            const ImVec2 cts = ImGui::CalcTextSize(cnt);
            const float ph = cts.y + 2.f;
            const float pw = std::max(cts.x + 8.f, ph);
            const ImVec2 pmax(ImGui::GetItemRectMin().x - 6.f, head_mid_y + ph * .5f);
            const ImVec2 pmin(pmax.x - pw, pmax.y - ph);
            dl->AddRectFilled(pmin, pmax, ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.22f)), ph * .5f);
            dl->AddText(ImVec2(pmin.x + (pw - cts.x) * .5f, pmin.y + (ph - cts.y) * .5f),
                        ImGui::GetColorU32(ImGuiCol_Text), cnt);
        }

        if (!open)
        {
            ImGui::Dummy(ImVec2(0.f, 2.f));
            return;
        }

        // --- rows, hung off a vertical timeline rail --------------------------------
        // The rail is drawn *before* the rows, from a predicted length, so the per-row dots
        // land on top of the line instead of under it. Row height is fixed (GetFrameHeight,
        // the same value passed to Selectable), so the prediction is exact.
        const float row_h = ImGui::GetFrameHeight();
        const ImVec2 rail_origin = ImGui::GetCursorScreenPos(); // already indented by TreePush
        const float rail_x = rail_origin.x + 6.f;
        const float rail_len =
            static_cast<float>(rows.size()) * row_h + static_cast<float>(rows.size() - 1) * imstyle.ItemSpacing.y;
        dl->AddLine(ImVec2(rail_x, rail_origin.y + row_h * .5f),
                    ImVec2(rail_x, rail_origin.y + rail_len - row_h * .5f),
                    ImGui::GetColorU32(ImGuiCol_Separator), 1.5f);

        for (const std::shared_ptr<avR::AvRequest> &snap : rows)
            this->render_history_row(this->shared_state->display_request, snap, imstyle, rail_x);

        ImGui::TreePop();
        ImGui::Dummy(ImVec2(0.f, 4.f));
    }

    void RequstListViewUi::render_history_row(const avR::AvRequest *selected,
                                              const std::shared_ptr<avR::AvRequest> &snap, const ImGuiStyle &imstyle,
                                              float rail_x)
    {
        avR::UiScopedId scid(snap.get());

        // Pointer identity, never id: a snapshot has id 0, and comparing ids against the
        // origin made the saved row and every one of its snapshots highlight at once.
        const bool is_selected = (selected == snap.get());
        const int code = snap->status_code.value_or(0);
        const ImVec4 status_col = this->get_status_color(code);
        const float row_h = ImGui::GetFrameHeight();

        if (ImGui::Selectable("##snap", is_selected, ImGuiSelectableFlags_None, ImVec2(0.f, row_h)))
            this->select_snapshot(snap);

        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImVec2 rmax = ImGui::GetItemRectMax();
        const float mid_y = (rmin.y + rmax.y) * .5f;
        ImDrawList *dl = ImGui::GetWindowDrawList();

        // timeline dot
        dl->AddCircleFilled(
            ImVec2(rail_x, mid_y), (is_selected || hovered) ? 4.5f : 3.f,
            ImGui::GetColorU32(is_selected ? status_col
                                           : ImVec4(status_col.x, status_col.y, status_col.z, 0.60f)));

        const float text_h = ImGui::GetTextLineHeight();
        const float text_y = mid_y - text_h * .5f;
        float x = rail_x + 12.f;

        // status pill
        char code_buf[8];
        if (code > 0)
            std::snprintf(code_buf, sizeof(code_buf), "%d", code);
        else
            std::snprintf(code_buf, sizeof(code_buf), "---");
        const ImVec2 code_sz = ImGui::CalcTextSize(code_buf);
        const float pill_h = text_h + 3.f;
        const float pill_w = 42.f;
        const ImVec2 pmin(x, mid_y - pill_h * .5f);
        const ImVec2 pmax(x + pill_w, mid_y + pill_h * .5f);
        dl->AddRectFilled(pmin, pmax, ImGui::GetColorU32(ImVec4(status_col.x, status_col.y, status_col.z, 0.16f)),
                          pill_h * .5f);
        dl->AddRect(pmin, pmax, ImGui::GetColorU32(ImVec4(status_col.x, status_col.y, status_col.z, 0.55f)),
                    pill_h * .5f);
        dl->AddText(ImVec2(pmin.x + (pill_w - code_sz.x) * .5f, text_y), ImGui::GetColorU32(status_col), code_buf);
        x = pmax.x + 10.f;

        // time of day
        const std::string clock = time_of_day(this->root.timestamp_to_date(snap->timestamp));
        dl->AddText(ImVec2(x, text_y), ImGui::GetColorU32(ImGuiCol_Text), clock.c_str());
        x += ImGui::CalcTextSize(clock.c_str()).x + 12.f;

        // elapsed
        char ms_buf[32];
        std::snprintf(ms_buf, sizeof(ms_buf), ICON_FA_STOPWATCH "  %lld ms",
                      static_cast<long long>(snap->last_result.elapsed_mc / 1000));
        dl->AddText(ImVec2(x, text_y), ImGui::GetColorU32(ImGuiCol_TextDisabled), ms_buf);

        // body size, right-aligned
        const std::string size_txt = format_bytes(snap->last_result.body.size());
        dl->AddText(ImVec2(rmax.x - ImGui::CalcTextSize(size_txt.c_str()).x - imstyle.ItemSpacing.x, text_y),
                    ImGui::GetColorU32(ImGuiCol_TextDisabled), size_txt.c_str());

        if (hovered)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(this->get_method_color(snap->method), "%s",
                               avNet::NetworkManager::method_text(snap->method));
            ImGui::SameLine();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.f);
            ImGui::TextUnformatted(snap->url.c_str());
            ImGui::PopTextWrapPos();
            ImGui::TextDisabled("%s  -  %s", this->root.timestamp_to_date(snap->timestamp).c_str(),
                                avNet::NetworkManager::status_text(snap->last_result.status));
            ImGui::EndTooltip();
        }

        if (ImGui::BeginPopupContextItem("##snap_ctx"))
        {
            ImGui::TextDisabled("%s", this->root.timestamp_to_date(snap->timestamp).c_str());
            ImGui::Separator();
            if (ImGui::Selectable(ICON_FA_LINK "  copy url"))
                ImGui::SetClipboardText(snap->url.c_str());
            if (ImGui::Selectable(ICON_FA_FILE_LINES "  copy response"))
                ImGui::SetClipboardText(snap->last_result.body.c_str());
            if (ImGui::Selectable(ICON_FA_TRASH "  remove from history"))
                this->pending_history_action = {HistoryAction::Kind::remove, snap->snapshot_of, snap->snapshot_id};
            ImGui::EndPopup();
        }
    }

    void RequstListViewUi::render_tab_saved(const ImGuiStyle &imstyle)
    {
        // #TODO add collection here
        const avR::AvRequest *selected = this->shared_state->display_request;

        auto todaysRequests =
            this->request_list_state->requests | std::views::filter([this](const std::shared_ptr<avR::AvRequest> &req)
                                                                    { return root.is_today(req->timestamp); });
        if (!todaysRequests.empty())
        {
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Indent(12.f);
            ImGui::TextDisabled("Today");
            ImGui::Unindent(12.f);
            ImGui::Dummy(ImVec2(0.0f, 5.0f));

            for (std::shared_ptr<avR::AvRequest> &request : todaysRequests)
            {
                this->render_request_row(selected, request.get(), imstyle);
            }
            ImGui::Dummy(ImVec2(0.0f, 5.0f));
            ImGui::Separator();
        }
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        for (std::shared_ptr<avR::AvRequest> &request :
             this->request_list_state->requests | std::views::filter([this](const std::shared_ptr<avR::AvRequest> &req)
                                                                     { return !root.is_today(req->timestamp); }))
        {
            this->render_request_row(selected, request.get(), imstyle);
        }
    }

    void RequstListViewUi::render_request_row(const avR::AvRequest *selected, avR::AvRequest *request,
                                              const ImGuiStyle &imstyle)
    {
        avR::UiScopedId scid(request);
        // pointer identity, not id: comparing ids highlighted the saved row whenever one of
        // its history snapshots was displayed (and vice versa).
        const bool is_selected = (selected == request);
        const ImVec2 row_start = ImGui::GetCursorPos();
        const float row_width = ImGui::GetContentRegionAvail().x;
        const float pad_x = imstyle.ItemSpacing.x;
        const float line_h = ImGui::GetTextLineHeight();

        // --- line 1 column geometry: [METHOD]  [wrapped title]  [STATUS] ---
        const char *method_txt = avNet::NetworkManager::method_text(request->method);
        const float method_w = ImGui::CalcTextSize(method_txt).x;

        char status_buf[8];
        std::snprintf(status_buf, sizeof(status_buf), "%d", request->status_code.value_or(0));
        const float status_w = ImGui::CalcTextSize(status_buf).x;

        const std::string title_txt = request->display_name(); // copy: unambiguous lifetime

        const float title_x = row_start.x + pad_x + method_w + imstyle.ItemSpacing.x;
        const float status_x = row_start.x + row_width - status_w - pad_x;
        const float title_right = status_x - imstyle.ItemSpacing.x;           // absolute wrap X
        const float title_wrap_width = std::max(title_right - title_x, 1.0f); // matching width
        const float title_h =
            std::max(ImGui::CalcTextSize(title_txt.c_str(), nullptr, false, title_wrap_width).y, line_h);

        // --- url geometry (line 2+) ---
        const float wrap_width = row_width - pad_x * 2.0f;
        const float url_h = ImGui::CalcTextSize(request->url.c_str(), nullptr, false, wrap_width).y;

        // line 1 is as tall as the (possibly multi-line) title
        const float row_height = imstyle.FramePadding.y * 2.0f + title_h + imstyle.ItemSpacing.y + url_h;

        // 1) clickable box, dynamic height
        if (ImGui::Selectable("##row", is_selected, ImGuiSelectableFlags_None, ImVec2(0.0f, row_height)))
        {
            this->select_request(request);
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("REQ", &request, sizeof(request));
            ImGui::Text(ICON_FA_GRIP_VERTICAL "  %s", request->display_name().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("REQ"))
            {
                avR::AvRequest *dropedReq = *(avR::AvRequest **)payload->Data;
                if (try_swap(this->request_list_state->requests, request, dropedReq))
                {
                    request->timestamp = this->root.get_timestamp();
                    dropedReq->timestamp = this->root.get_timestamp();
                    apply_reordering(this->request_list_state->requests);
                    this->request_storage->upsert(this->request_list_state->requests);
                }
            }

            ImGui::EndDragDropTarget();
        }

        const ImVec2 row_end = ImGui::GetCursorPos();
        // 2) right-click -> title edit panel (must stay right after the selectable)
        if (ImGui::BeginPopupContextItem("##context_popup"))
        {
            // ImGui::SetNextItemWidth(200.0f);
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            if (!request->title.has_value())
                request->title.emplace(request->display_name());
            ImGui::TextDisabled("%s", ("last modified: " + this->root.timestamp_to_date(request->timestamp)).c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled(ICON_FA_PEN "  title");
            ImGui::SameLine();
            if (ImGui::InputText("##title", &request->title.value(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                this->request_storage->upsert(request);
                ImGui::CloseCurrentPopup();
            }
            ImGui::Spacing();
            if (ImGui::Button(ICON_FA_TRASH "  delete"))
            {
                this->pending_delete_req = request->id;
                // this->request_storage->del(request->id);
                // std::erase_if(this->request_list_state->requests,
                //               [id = request->id](std::shared_ptr<avR::AvRequest> x) { return x->id == id; });
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // 3) line 1: METHOD (top-left), STATUS (top-right), wrapped TITLE (middle column)
        const float line1_y = row_start.y + imstyle.FramePadding.y;

        ImGui::SetCursorPos(ImVec2(row_start.x + pad_x, line1_y));
        ImGui::TextColored(this->get_method_color(request->method), "%s", method_txt);

        ImGui::SetCursorPos(ImVec2(status_x, line1_y));
        if (request->status_code.value_or(0) != 0)
            ImGui::TextColored(this->get_status_color(request->status_code.value()), "%s", status_buf);

        ImGui::SetCursorPos(ImVec2(title_x, line1_y));
        ImGui::PushTextWrapPos(title_right);
        ImGui::TextUnformatted(title_txt.c_str());
        ImGui::PopTextWrapPos();

        // 4) line 2+: wrapped url
        ImGui::SetCursorPos(ImVec2(row_start.x + pad_x, line1_y + title_h + imstyle.ItemSpacing.y));
        ImGui::PushTextWrapPos(row_start.x + row_width - pad_x);
        ImGui::TextDisabled("%s", request->url.c_str());
        ImGui::PopTextWrapPos();

        // 5) restore layout cursor
        ImGui::SetCursorPos(row_end);
    }

    void RequstListViewUi::new_request()
    {
        std::shared_ptr<avR::AvRequest> req = std::make_shared<avR::AvRequest>(avR::AvRequest{
            .timestamp = this->root.get_timestamp(),
        });
        this->request_list_state->requests.push_back(std::move(req));
        apply_reordering(this->request_list_state->requests);
        this->request_storage->upsert(this->request_list_state->requests);
        this->select_request(this->request_list_state->requests.back().get());
    }

    void RequstListViewUi::select_snapshot(const std::shared_ptr<avR::AvRequest> &snap)
    {
        // Hold the snapshot for as long as it is displayed: it lives in the origin's
        // recent_reqs, which trimming, "clear" and deleting the origin all shrink underneath
        // the raw display_request pointer.
        this->shared_state->display_request_hold = snap;
        this->shared_state->display_request = snap.get();
        if (this->shared_state->on_display_request_change.has_value())
            this->shared_state->on_display_request_change.value()();
    }

    void RequstListViewUi::select_request(avR::AvRequest *request)
    {
        // saved requests are owned by request_list_state->requests - release the snapshot
        // reference so a history entry is not kept alive for nothing.
        this->shared_state->display_request_hold.reset();
        this->shared_state->display_request = request;
        if (this->shared_state->on_display_request_change.has_value())
            this->shared_state->on_display_request_change.value()();
    }

    void RequstListViewUi::apply_history_action()
    {
        if (this->pending_history_action.kind == HistoryAction::Kind::none)
            return;

        const HistoryAction action = this->pending_history_action;
        this->pending_history_action = {};

        const auto it = std::ranges::find_if(this->request_list_state->requests,
                                             [&action](const std::shared_ptr<avR::AvRequest> &r)
                                             { return r->id == action.origin_id; });
        if (it == this->request_list_state->requests.end())
            return;

        avR::AvRequest *origin = it->get();

        // display_request_hold keeps a displayed-but-removed snapshot alive, so the detailed
        // view stays valid; the entry simply disappears from the tree.
        switch (action.kind)
        {
        case HistoryAction::Kind::clear:
            origin->recent_reqs.clear();
            break;
        case HistoryAction::Kind::remove:
            std::erase_if(origin->recent_reqs, [&action](const std::shared_ptr<avR::AvRequest> &s)
                          { return s->snapshot_id == action.snapshot_id; });
            break;
        case HistoryAction::Kind::none:
            break;
        }
    }

    void RequstListViewUi::apply_pending_delete()
    {
        if (!this->pending_delete_req.has_value())
            return;

        const int64_t req_id = this->pending_delete_req.value();

        // A deleted request takes its history with it. Drop the selection when it is the
        // request itself *or* any snapshot taken from it, so display_request never points
        // into a vector that is about to be freed. (A snapshot has id 0 and a saved request
        // never does, so the id test cannot false-positive.)
        const avR::AvRequest *shown = this->shared_state->display_request;
        if (shown && (shown->id == req_id || shown->snapshot_of == req_id))
        {
            this->shared_state->display_request = nullptr;
            this->shared_state->display_request_hold.reset();
        }

        this->request_storage->del(req_id);
        std::erase_if(this->request_list_state->requests,
                      [req_id](const std::shared_ptr<avR::AvRequest> &x) { return x->id == req_id; });

        this->pending_delete_req.reset();
    }
} // namespace avUi
