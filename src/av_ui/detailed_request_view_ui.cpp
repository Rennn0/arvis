#include <av_ui/detailed_request_view_ui.hpp>
#include <av_ui/json_tree_view.hpp>
#include <av_ui/input_autocomplete_ui.hpp>
#include <av_ui/icons_font_awesome7.hpp>
#include <string>
#include <cstdlib>
#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif
namespace avUi
{
    std::optional<int64_t> pendingParamDel;
    std::optional<int64_t> pendingHeaderDel;
    std::optional<int64_t> pendingCookieDel;
    bool switchStyles = false;
    bool switchShortcuts = false;
    bool tabParamBadgeEnabled = true;
    bool tabHeaderBadgeEnabled = true;
    bool tabCookieBadgeEnabled = true;
    int tabParamCountBadge = 0;
    int tabHeaderCountBadge = 0;
    int tabCookieCountBadge = 0;

    /// @brief Next synthetic row id for an editable history snapshot. Rows on a snapshot are
    ///        addressed by distinct *negative* ids: SQLite rowids are always positive, so a
    ///        negative id can never reach a persisted row, and staying distinct keeps the
    ///        id-based row erase from removing every row at once (a shared 0 would).
    template <typename T> static int64_t next_scratch_row_id(const std::vector<T> &rows)
    {
        int64_t lowest = 0;
        for (const T &row : rows)
            lowest = std::min(lowest, row.id);
        return lowest - 1;
    }

    EnvVars envVars;
    void load_env_vars(avR::AvEnvironment *env)
    {
        envVars = {};
        for (const avR::AvEnvironmentVariable &var : env->vars)
        {
            envVars.emplace_back(avUi::Var{var.key, var.value});
        }
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id)
        : avR::UiComponent(std::move(id)), footer_height(-1.f),
          request_storage(std::make_unique<avS::AvRequestStorage>()),
          request_params_storage(std::make_unique<avS::AvRequestParamsStorage>()),
          request_headers_storage(std::make_unique<avS::AvRequestHeadersStorage>()),
          request_cookies_storage(std::make_unique<avS::AvRequestCookiesStorage>()),
          _tabs(std::make_unique<avUi::TabBarUi>("tabs", 15.f, 3.f, avUi::TabBarUi::Sizing::fit, 90.f)),
          network_manager(this->request_storage->get_db_path()), json_view(std::make_unique<JsonTreeView>())
    {
        this->window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id, avR::AvState *sharedState) : DetailedRequestViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
        this->shared_state->on_display_request_change.emplace(
            [this]()
            {
                avR::AvRequest *shown = this->shared_state->display_request;
                if (!shown)
                    return;

                // A history snapshot is self-contained and owns no DB row: it must never be
                // refreshed from storage. Reloading by id here (a snapshot used to carry the
                // origin's id) is what made a snapshot display the saved request's *current*
                // params/headers/cookies instead of the ones it captured at send time.
                if (!shown->is_recent)
                {
                    const int64_t id = shown->id;
                    shown->params = this->request_params_storage->select_by_req_id(id);
                    shown->headers = this->request_headers_storage->select_by_req_id(id);
                    shown->cookies = this->request_cookies_storage->select_by_req_id(id);
                }

                this->json_view->set_source(shown->last_result.body);
                this->response_view = this->json_view->is_json() ? ResponseView::tree : ResponseView::raw;
            });

        this->shared_state->on_send_request.emplace([this]() { this->send_request(); });
        this->shared_state->on_save_changes.emplace([this]() { this->save_changes(); });
        this->shared_state->on_show_shortcuts.emplace([this]() { switchShortcuts = !switchShortcuts; });
        this->shared_state->on_show_style_editor.emplace([this]() { switchStyles = !switchStyles; });
        this->shared_state->on_env_change.emplace(
            [this]() { load_env_vars(this->shared_state->request_list_state->env.get()); });
        if (!this->shared_state->is_init)
            this->shared_state->on_display_request_change.value()();

        load_env_vars(this->shared_state->request_list_state->env.get());

        _tabs->addTab(ICON_FA_LINK "  params", &tabParamBadgeEnabled, &tabParamCountBadge);
        _tabs->addTab(ICON_FA_LIST "  headers", &tabHeaderBadgeEnabled, &tabHeaderCountBadge);
        _tabs->addTab(ICON_FA_COOKIE_BITE "  cookies", &tabCookieBadgeEnabled, &tabCookieCountBadge);
        _tabs->addTab(ICON_FA_CODE "  body");
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
        // never let the worker outlive this component (and its NetworkManager / response_body).
        if (this->pending_response_v2.valid())
            this->pending_response_v2.wait();
    }

    void DetailedRequestViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_detailed_view)
            return;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const float x = viewport->WorkSize.x;
        const float y = viewport->WorkSize.y;
        const bool listVisible = this->shared_state && this->shared_state->show_req_list_view;
        const float leftW =
            listVisible ? std::max(this->shared_state->_min_left_panel_width, x * this->shared_state->_left_panel_ratio)
                        : 0.f;
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + leftW, viewport->WorkPos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(x - leftW, y), ImGuiCond_Always);

        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_detailed_view, this->window_flags))
        {
            this->shared_state->shortcutManager.process();

            // pick up a completed request once per frame so the header (button state) and
            // footer (response) agree within the same frame. Runs before the "nothing
            // selected" early-out: the response must still be routed to the request that
            // was sent even if the user cleared the selection while it was in flight.
            this->poll_response();

            avR::AvRequest *displayReq = this->shared_state->display_request;
            if (!displayReq)
            {
                const char *msg = ICON_FA_INBOX "  No request selected";
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const ImVec2 sz = ImGui::CalcTextSize(msg);
                ImGui::SetCursorPos(ImVec2((avail.x - sz.x) * .5f, (avail.y - sz.y) * .5f));
                ImGui::TextDisabled("%s", msg);
                ImGui::End();
                return;
            }

            this->render_menu();

            const ImGuiStyle &imstyle = ImGui::GetStyle();
            const ImVec2 availRegion = ImGui::GetContentRegionAvail();

            if (this->footer_height < 0.f)
                this->footer_height = availRegion.y * .6f;

            const float splitterThikness = 3.f;
            const float minFooter = 50.f;
            const float maxFooter = availRegion.y - 100.f;
            this->footer_height = std::clamp(this->footer_height, minFooter, maxFooter);
            const float headerHeight = (availRegion.y - this->footer_height) * .1f;

            ImGui::BeginChild("header", ImVec2(0, headerHeight));
            this->render_header(imstyle);
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::BeginChild("main_content", ImVec2(0, -(this->footer_height + splitterThikness)));
            this->render_main_content(imstyle);
            ImGui::EndChild();

            ImGui::InvisibleButton("##splitter", ImVec2(-1.f, splitterThikness));
            if (ImGui::IsItemActive())
            {
                this->footer_height -= ImGui::GetIO().MouseDelta.y;
                this->footer_height = std::clamp(this->footer_height, minFooter, maxFooter);
            }
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }

            {
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                float midY = (pMax.y + pMin.y) * .5f;
                ImU32 col = ImGui::GetColorU32(ImGui::IsItemActive()    ? ImGuiCol_SeparatorActive
                                               : ImGui::IsItemHovered() ? ImGuiCol_SeparatorHovered
                                                                        : ImGuiCol_Separator);

                ImGui::GetWindowDrawList()->AddLine(ImVec2(pMin.x, midY), ImVec2(pMax.x, midY), col, splitterThikness);
            }

            ImGui::BeginChild("footer", ImVec2(0, 0));
            this->render_footer(imstyle);
            ImGui::EndChild();

            this->render_shortcuts();
        }

        ImGui::End();

        if (switchStyles)
        {
            if (ImGui::Begin("style editor", &switchStyles))
            {
                ImGui::ShowStyleEditor();
            }

            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                switchStyles = false;
            }
            ImGui::End();
        }
    }

    void DetailedRequestViewUi::update()
    {
        const avR::AvRequest *displayReq = this->shared_state->display_request;
        if (!displayReq)
            return;
        tabParamCountBadge = displayReq->params.size();
        tabHeaderCountBadge = displayReq->headers.size();
        tabCookieCountBadge = displayReq->cookies.size();
    }

    void DetailedRequestViewUi::render_header(const ImGuiStyle &style)
    {
        avR::AvRequest &req = *this->shared_state->display_request;
        if (!req.title.has_value())
            req.title.emplace(req.display_name());

        // A history snapshot is an editable replay scratch pad: it can be modified and sent
        // again, but it owns no DB row, so nothing about it is ever persisted and the saved
        // request it was taken from is never touched.
        const bool scratch = req.is_recent;

        const float rowH = ImGui::GetFrameHeight();
        const float availH = ImGui::GetContentRegionAvail().y;
        if (availH > rowH)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (availH - rowH) * 0.5f);

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);
        ImGui::AlignTextToFramePadding();

        ImGui::PushStyleColor(ImGuiCol_Text, this->get_method_color(req.method));
        const char *current = avNet::NetworkManager::method_text(req.method);
        ImGui::SetNextItemWidth(ImGui::CalcTextSize(current).x + style.FramePadding.x * 2.0f + ImGui::GetFrameHeight());

        if (ImGui::BeginCombo("##method", current))
        {
            static constexpr avNet::request_method methods[] = {
                avNet::request_method::get,  avNet::request_method::post,  avNet::request_method::put,
                avNet::request_method::del,  avNet::request_method::patch, avNet::request_method::options,
                avNet::request_method::head,
            };

            for (avNet::request_method m : methods)
            {
                const bool selected = (req.method == m);
                ImGui::PushStyleColor(ImGuiCol_Text, this->get_method_color(m));
                if (ImGui::Selectable(avNet::NetworkManager::method_text(m), selected))
                {
                    req.method = m;
                    this->save_state_change();
                }
                ImGui::PopStyleColor();
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);
        if (avUi::InputTextAutocomplete("##title_edit", &req.url, envVars,
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            this->save_state_change();
        };

        ImGui::SameLine();
        const bool in_flight = this->pending_response_v2.valid();
        ImGui::BeginDisabled(in_flight);
        if (ImGui::Button(in_flight ? ICON_FA_HOURGLASS_HALF " Sending..." : ICON_FA_PAPER_PLANE " Send"))
        {
            this->send_request();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", this->root.timestamp_to_date(req.timestamp).c_str());

        if (scratch)
        {
            ImGui::SameLine();
            avR::UiScopedStyle chip;
            chip.color(ImGuiCol_Text, ImVec4(0.62f, 0.68f, 0.78f, 1.0f));
            ImGui::TextUnformatted(ICON_FA_CLOCK_ROTATE_LEFT "  history");
            ImGui::SetItemTooltip("snapshot of a past send - editable and re-sendable, but never "
                                  "saved, and independent of the request it came from");
        }
        else if (req.pending_save)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(ICON_FA_CIRCLE_DOT);
            ImGui::SetItemTooltip("unsaved changes - press ctrl+s to save");
        }
    }
    void DetailedRequestViewUi::render_main_content(const ImGuiStyle &imstyle)
    {
        this->_tabs->draw();
        ImGui::Spacing();

        // Snapshots are editable here. What keeps their edits off the origin's DB rows is not
        // a disabled flag but their ids: make_snapshot rewrites every row id to a distinct
        // negative value, and the delete paths below skip storage entirely for a snapshot.
        switch (this->_tabs->getActiveTab())
        {
        case 0:
            this->render_tab_params();
            break;
        case 1:
            this->render_tab_headers();
            break;
        case 2:
            this->render_tab_cookies();
            break;
        case 3:
            this->render_tab_body();
            break;
        default:
            this->render_tab_params();
            break;
        }
    }
    // wrap a value in shell single quotes, escaping any embedded single quote as '\'' so the
    // result is safe to paste into a POSIX shell.
    static std::string shell_single_quote(std::string_view s)
    {
        std::string out;
        out.reserve(s.size() + 2);
        out.push_back('\'');
        for (char c : s)
        {
            if (c == '\'')
                out += "'\\''";
            else
                out.push_back(c);
        }
        out.push_back('\'');
        return out;
    }

    // render the exact request we sent as a copy-pasteable `curl` command line.
    static std::string format_as_curl(const avNet::http_request &req)
    {
        std::string method = avNet::NetworkManager::method_text(req.method);
        for (char &c : method)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        std::string out = "curl -X ";
        out += method;
        out += ' ';
        out += shell_single_quote(req.url);

        for (const auto &[key, value] : req.headers)
        {
            out += " -H ";
            out += shell_single_quote(key + ": " + value);
        }

        // cookies go out as one `-b "a=1; b=2"` just like the single Cookie line we send.
        std::string cookies;
        for (const auto &[name, value] : req.cookies)
        {
            if (name.empty())
                continue;
            if (!cookies.empty())
                cookies += "; ";
            cookies += name + "=" + value;
        }
        if (!cookies.empty())
        {
            out += " -b ";
            out += shell_single_quote(cookies);
        }

        if (req.body && !req.body->empty())
        {
            out += " --data ";
            out += shell_single_quote(*req.body);
        }

        return out;
    }

    static void render_kv_table(const char *id, const std::vector<std::pair<std::string, std::string>> &rows)
    {
        using namespace ImGui;
        avR::UiScopedStyle scoped(avR::UiScopedStyle::Style{.frame_rounding = 0, .frame_border = 0});
        const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY;
        if (!BeginTable(id, 2, flags, ImVec2(0, 0), 0.))
            return;
        TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch);
        TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
        TableSetupScrollFreeze(0, 1);
        TableHeadersRow();
        for (const auto &pair : rows)
        {
            PushID(&pair);
            TableNextRow();
            TableNextColumn();
            AlignTextToFramePadding();
            TextUnformatted(pair.first.c_str());
            TableNextColumn();
            TextWrapped("%s", pair.second.c_str());
            PopID();
        }
        EndTable();
    }

    void DetailedRequestViewUi::render_footer(const ImGuiStyle &style)
    {
        if (this->pending_response_v2.valid())
        {
            ImGui::TextDisabled(ICON_FA_HOURGLASS_HALF "  Sending request...");
            return;
        }

        if (this->shared_state->display_request->last_result.body.empty() &&
            this->shared_state->display_request->last_result.http_code == 0)
        {
            ImGui::TextDisabled(ICON_FA_INBOX "  No response yet - press Send to run the request.");
            return;
        }

        const bool ok = this->shared_state->display_request->last_result.status == avNet::response_status::Ok;
        const ImVec4 statusColor = ok ? ImVec4(0.40f, 0.80f, 0.40f, 1.f) : ImVec4(0.90f, 0.40f, 0.40f, 1.f);

        this->shared_state->display_request->status_code = this->shared_state->display_request->last_result.http_code;

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(statusColor, "%s  %s", ok ? ICON_FA_CIRCLE_CHECK : ICON_FA_TRIANGLE_EXCLAMATION,
                           avNet::NetworkManager::status_text(this->shared_state->display_request->last_result.status));
        ImGui::SameLine();
        ImGui::TextDisabled("HTTP %ld", this->shared_state->display_request->last_result.http_code);
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu bytes)", this->shared_state->display_request->last_result.body.size());

        // copy options sit next to the status line and act on the request we actually sent
        // (last_request) and the response we got back (response_body).
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_COPY "  copy"))
            ImGui::OpenPopup("##copy_options");
        if (ImGui::BeginPopup("##copy_options"))
        {
            if (ImGui::Selectable(ICON_FA_TERMINAL "  copy req as curl"))
                ImGui::SetClipboardText(format_as_curl(this->shared_state->display_request->last_request).c_str());
            if (ImGui::Selectable(ICON_FA_FILE_LINES "  copy raw response"))
                ImGui::SetClipboardText(this->shared_state->display_request->last_result.body.c_str());
            if (ImGui::Selectable(ICON_FA_LINK "  copy request url"))
                ImGui::SetClipboardText(this->shared_state->display_request->last_request.url.c_str());
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::Text(ICON_FA_CLOCK "  %lld ms",
                    static_cast<long long>(this->shared_state->display_request->last_result.elapsed_mc / 1000));

        ImGui::Separator();
        ImGui::Spacing();

        // JSON bodies get the interactive tree / pretty views; anything else (HTML, plain text,
        // an error string) falls back to the raw text view. The selector only appears when there
        // is a tree/pretty view to switch to.
        const bool json = this->json_view->is_json();
        const auto mode_tab = [&](const char *label, ResponseView mode, unsigned short count = 0)
        {
            const bool active = this->response_view == mode;
            if (active)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            if (ImGui::Button(label))
                this->response_view = mode;
            if (active)
                ImGui::PopStyleColor();
            if (count < 1)
                return;
            char buf[16];
            if (count > 99)
                snprintf(buf, sizeof(buf), "99+");
            else
                snprintf(buf, sizeof(buf), "%d", count);

            ImDrawList *dl = ImGui::GetWindowDrawList();
            const ImVec2 ts = ImGui::CalcTextSize(buf);

            const float pad_x = 3.f;
            const float h = ts.y;
            const float w = (ts.x + pad_x * 2.0f > h) ? ts.x + pad_x * 2.0f : h; // pill, min circular
            const ImVec2 topRight = ImVec2(ImGui::GetItemRectMax().x, ImGui::GetItemRectMin().y);
            const ImVec2 bMin = ImVec2(topRight.x - w * .5, topRight.y - h * .5);
            const ImVec2 bMax = ImVec2(bMin.x + w, bMin.y + h);
            dl->AddRectFilled(bMin, bMax, IM_COL32(120, 130, 145, 180), h * 0.5f); // rounded = pill
            dl->AddText(ImVec2(bMin.x + (w - ts.x) * 0.5f, bMin.y + (h - ts.y) * 0.5f), IM_COL32_WHITE, buf);
        };
        const avR::AvRequest *rp = this->shared_state->display_request;
        if (json)
        {
            mode_tab(ICON_FA_SITEMAP "  Tree", ResponseView::tree);
            ImGui::SameLine();
            mode_tab(ICON_FA_CODE "  Pretty", ResponseView::pretty);
        }
        ImGui::SameLine();
        mode_tab(ICON_FA_FILE_LINES "  Raw", ResponseView::raw);
        ImGui::SameLine();
        mode_tab(ICON_FA_LIST "  Response Headers", ResponseView::res_headers,
                 static_cast<unsigned short>(rp->last_result.response_headers.size()));
        ImGui::SameLine();
        mode_tab(ICON_FA_COOKIE_BITE "  Response Cookies", ResponseView::res_cookies,
                 static_cast<unsigned short>(rp->last_result.response_cookies.size()));
        if (this->response_view == ResponseView::res_headers)
        {
            render_kv_table("res_headers", rp->last_result.response_headers);
        }
        else if (this->response_view == ResponseView::res_cookies)
        {
            render_kv_table("res_cookies", rp->last_result.response_cookies);
        }
        else if (json && this->response_view == ResponseView::tree)
        {
            this->json_view->render_tree();
        }
        else if (json && this->response_view == ResponseView::pretty)
        {
            this->json_view->render_pretty();
        }
        else
        {
            ImGui::InputTextMultiline("##raw", &const_cast<avR::AvRequest *>(rp)->last_result.body,
                                      ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);
        }
    }

    void DetailedRequestViewUi::send_request()
    {
        if (!this->shared_state || !this->shared_state->display_request)
            return;

        // one request at a time; ignore the button while a fetch is in flight.
        if (this->pending_response_v2.valid())
            return;

        avR::AvRequest *req = this->shared_state->display_request;
        req->timestamp = this->root.get_timestamp();

        // A snapshot can be edited and sent again. Re-running one updates that history entry
        // in place instead of appending another, so replaying an entry does not breed a new
        // one on every send; and it is never marked dirty, since it has nothing to save to.
        const bool replay = req->is_recent;
        if (!replay)
            req->pending_save = true;

        // params are the source of truth for the query string: assemble the final url from
        // them, reflect it back into the editable url, and persist.
        std::string url = build_url(req->url, req->params);
        req->url = url;
        this->save_state_change();

        // snapshot the request (method + url + included headers + cookies + body) into a
        // self-contained descriptor the worker owns. It copies the header/cookie strings, so the
        // worker never races against edits to display_request on the UI thread while the fetch
        // is in flight.
        avNet::http_request request;
        request.method = req->method;
        request.url = avUi::resolve_vars(url, envVars);
        request.body = req->body;
        for (const avR::AvRequestHeader &header : req->headers)
        {
            if (!header.included || header.key.empty())
                continue;
            request.headers.emplace_back(header.key, avUi::resolve_vars(header.value, envVars));
        }
        for (const avR::AvRequestCookie &cookie : req->cookies)
        {
            if (!cookie.included || cookie.key.empty())
                continue;
            request.cookies.emplace_back(cookie.key, avUi::resolve_vars(cookie.value, envVars));
        }

        // keep an independent copy of exactly what we send so the footer can reproduce it
        // (copy as cURL, etc.) without touching the worker's moved-in copy.
        req->last_request = request;

        if (replay)
        {
            // Re-running a history entry: the result goes back into that same entry. It is
            // held by shared_ptr so trimming or clearing history mid-flight cannot free it
            // under us. select_snapshot is the only way a snapshot reaches display_request,
            // and it always sets display_request_hold, so this is the owning reference.
            this->pending_replay_target = this->shared_state->display_request_hold;
            this->pending_snapshot.reset();
            this->pending_snapshot_origin_id = 0;
        }
        else
        {
            // Capture the history entry for this send. make_snapshot deep-copies the payload
            // and severs every link to the origin (id 0, its own snapshot_id, distinct
            // negative row ids, no nested history) - a plain `AvRequest tmp = *req` shares the
            // origin's id, which is what coupled the two. It is *held* here rather than
            // pushed, and lands in the tree in poll_response once there is a real response.
            this->pending_snapshot = avR::make_snapshot(*req, ++this->shared_state->recent_req_seq);
            this->pending_snapshot->last_request = request; // before `request` is moved below
            this->pending_snapshot_origin_id = req->id;
            this->pending_replay_target.reset();
        }

        // reset the destination on the UI thread before the worker starts writing to it.
        // std::launch::async establishes a happens-before with the worker; the UI thread only
        // reads response_body / response_http_code again after the future is ready (poll_response).
        req->last_result.body.clear();
        req->last_result.http_code = 0;

        // run off the UI thread so a slow/dead endpoint never freezes the window.
        /*this->pending_response*/ this->pending_response_v2 = std::async(
            std::launch::async, [this, request = std::move(request)]() { return this->network_manager.send(request); });
    }

    avR::AvRequest *DetailedRequestViewUi::find_saved_request(int64_t id) const
    {
        if (id == 0 || !this->shared_state->request_list_state)
            return nullptr;

        for (const std::shared_ptr<avR::AvRequest> &req : this->shared_state->request_list_state->requests)
        {
            if (req->id == id)
                return req.get();
        }
        return nullptr;
    }

    void DetailedRequestViewUi::trim_history(avR::AvRequest *origin) const
    {
        const size_t limit = this->shared_state->recent_req_limit;
        if (limit == 0 || origin->recent_reqs.size() <= limit)
            return;

        // Dropping the oldest entries destroys their shared_ptr. A snapshot the user is
        // currently looking at survives it because AvInterViewSharedState::display_request_hold
        // holds a strong reference for as long as display_request points at it.
        const size_t drop = origin->recent_reqs.size() - limit;
        origin->recent_reqs.erase(origin->recent_reqs.begin(),
                                  origin->recent_reqs.begin() + static_cast<std::ptrdiff_t>(drop));
    }

    void DetailedRequestViewUi::poll_response()
    {
        if (!this->pending_response_v2.valid() ||
            this->pending_response_v2.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        const avNet::http_result result = this->pending_response_v2.get();

        // Replay of an edited history entry: the result updates that entry in place.
        if (this->pending_replay_target)
        {
            this->pending_replay_target->last_result = result;
            this->pending_replay_target->status_code = static_cast<int>(result.http_code);
            this->pending_replay_target->timestamp = this->root.get_timestamp();
            this->pending_replay_target.reset();
        }

        // The response belongs to the request that was *sent*, not to whatever is on screen
        // now - the user may have selected another entry (or a snapshot) while it was in
        // flight. Look the origin up by id so a delete mid-flight cannot dangle.
        avR::AvRequest *origin = this->find_saved_request(this->pending_snapshot_origin_id);
        if (origin)
        {
            origin->last_result = result;
            origin->status_code = static_cast<int>(result.http_code);
        }

        // The snapshot gets its own copy of the result. Independent on purpose: the next send
        // clears origin->last_result, which would otherwise blank out the history entry.
        if (this->pending_snapshot)
        {
            this->pending_snapshot->last_result = result;
            this->pending_snapshot->status_code = static_cast<int>(result.http_code);
            this->pending_snapshot->timestamp = this->root.get_timestamp();

            if (origin)
            {
                origin->recent_reqs.push_back(std::move(this->pending_snapshot));
                this->trim_history(origin);
            }
            this->pending_snapshot.reset();
        }
        this->pending_snapshot_origin_id = 0;

        if (this->shared_state->display_request)
        {
            this->json_view->set_source(this->shared_state->display_request->last_result.body);
            this->response_view = this->json_view->is_json() ? ResponseView::tree : ResponseView::raw;
        }
    }

    void DetailedRequestViewUi::save_state_change() const
    {
        avR::AvRequest *shown = this->shared_state->display_request;
        if (!shown)
            return;

        // A snapshot owns no DB row, so it can never be dirty either - clearing the flag
        // stops the params tab from re-running build_url every frame on an edit that will
        // never be saved (and stops the "unsaved changes" marker from sticking forever).
        if (shown->is_recent)
        {
            shown->pending_save = false;
            return;
        }

        this->request_storage->upsert(shown);
    }

    void DetailedRequestViewUi::save_changes()
    {
        avR::AvRequest *shown = this->shared_state->display_request;
        if (!shown)
            return;

        // ctrl+s on a snapshot is deliberately a no-op: its edits are scratch by design.
        if (shown->is_recent)
        {
            shown->pending_save = false;
            return;
        }

        this->request_storage->upsert(shown);
        this->request_params_storage->upsert(shown->params);
        this->request_headers_storage->upsert(shown->headers);
        this->request_cookies_storage->upsert(shown->cookies);
        shown->pending_save = false;
    }

    // percent-encode per RFC 3986: unreserved chars pass through, everything else -> %XX
    static std::string url_encode(std::string_view s)
    {
        static constexpr char hex[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                c == '.' || c == '~')
            {
                out.push_back(static_cast<char>(c));
            }
            else
            {
                out.push_back('%');
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 0x0F]);
            }
        }
        return out;
    }

    // The params table is the source of truth for the query string: any existing "?..." on
    // base_url is dropped and rebuilt from the included params; a trailing "#fragment" is kept.
    std::string DetailedRequestViewUi::build_url(std::string_view base_url,
                                                 const std::vector<avR::AvRequestParam> &params)
    {
        std::string_view fragment;
        if (const size_t hash = base_url.find('#'); hash != std::string_view::npos)
        {
            fragment = base_url.substr(hash); // includes the '#'
            base_url = base_url.substr(0, hash);
        }

        if (const size_t query = base_url.find('?'); query != std::string_view::npos)
            base_url = base_url.substr(0, query);

        std::string url(base_url);

        bool first = true;
        for (const avR::AvRequestParam &p : params)
        {
            if (!p.included || p.key.empty())
                continue;

            url.push_back(first ? '?' : '&');
            first = false;

            url += url_encode(p.key);
            url.push_back('=');
            url += url_encode(avUi::resolve_vars(p.value, envVars));
        }

        url.append(fragment);
        return url;
    }
    void DetailedRequestViewUi::render_tab_params() const
    {
        avR::UiScopedStyle style;
        style.color(ImGuiCol_Button, this->tableXButtonColor);

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;

        if (ImGui::BeginTable("params_table", 5, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("include", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##delete", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            for (avR::AvRequestParam &item : this->shared_state->display_request->params)
            {
                ImGui::PushID(&item);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText("##key", &item.key);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                avUi::InputTextAutocomplete("##v", &item.value, envVars);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;

                ImGui::TableNextColumn();
                if (item.editing)
                {
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (item.set_focus)
                    {
                        ImGui::SetKeyboardFocusHere();
                        item.set_focus = false;
                    }
                    ImGui::InputText("##desc", &item.description);
                    if (ImGui::IsItemDeactivated())
                    {
                        item.editing = false;
                        this->shared_state->display_request->pending_save = true;
                    }
                }
                else
                {
                    ImVec2 start = ImGui::GetCursorPos();
                    float wrap = ImGui::GetContentRegionAvail().x;

                    ImGui::PushTextWrapPos(start.x + wrap);
                    ImGui::TextUnformatted(item.description.empty() ? " " : item.description.c_str());
                    ImGui::PopTextWrapPos();

                    float h = ImGui::GetItemRectSize().y;

                    ImGui::SetCursorPos(start);
                    if (ImGui::InvisibleButton("##desc_click", ImVec2(wrap, h)))
                    {
                        item.editing = true;
                        item.set_focus = true;
                    }
                }
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##included", &item.included))
                    this->shared_state->display_request->pending_save = true;
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_XMARK))
                    pendingParamDel = item.id;
                ImGui::SetItemTooltip("remove param");
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (ImGui::Button(ICON_FA_PLUS " add param"))
        {
            avR::AvRequest *req = this->shared_state->display_request;
            avR::AvRequestParam row{.request_id = req->id};
            if (req->is_recent)
                row.id = next_scratch_row_id(req->params);
            req->params.push_back(row);
            req->pending_save = true;
        }

        if (pendingParamDel.has_value())
        {
            const int64_t ppd = pendingParamDel.value();
            avR::AvRequest *req = this->shared_state->display_request;
            // a snapshot's rows are scratch (negative ids) - there is no DB row to delete
            if (!req->is_recent)
                this->request_params_storage->del(ppd);
            std::erase_if(req->params, [ppd](const avR::AvRequestParam &p) { return p.id == ppd; });
            pendingParamDel.reset();
        }

        // params are the source of truth for the query string: reflect any change back into the
        // url shown/edited in the header, and persist it.
        if (this->shared_state->display_request->pending_save)
        {
            avR::AvRequest *req = this->shared_state->display_request;
            req->url = build_url(req->url, req->params);
            this->save_state_change();
        }
    }
    void DetailedRequestViewUi::render_tab_headers() const
    {
        avR::UiScopedStyle scstyle;
        scstyle.color(ImGuiCol_Button, tableXButtonColor);

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;

        if (ImGui::BeginTable("tab_headers", 4, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("include", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##delete", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            for (avR::AvRequestHeader &header : this->shared_state->display_request->headers)
            {
                ImGui::PushID(&header);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText("##k", &header.key);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                avUi::InputTextAutocomplete("##v", &header.value, envVars);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##inc", &header.included))
                    this->shared_state->display_request->pending_save = true;

                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_XMARK))
                    pendingHeaderDel = header.id;
                ImGui::SetItemTooltip("remove header");
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        if (ImGui::Button(ICON_FA_PLUS "  add header"))
        {
            avR::AvRequest *req = this->shared_state->display_request;
            avR::AvRequestHeader row{avR::AvRequestParam{.request_id = req->id}};
            if (req->is_recent)
                row.id = next_scratch_row_id(req->headers);
            req->headers.push_back(row);
            req->pending_save = true;
        }

        if (pendingHeaderDel.has_value())
        {
            const int64_t phd = pendingHeaderDel.value();
            avR::AvRequest *req = this->shared_state->display_request;
            // a snapshot's rows are scratch (negative ids) - there is no DB row to delete
            if (!req->is_recent)
                this->request_headers_storage->del(phd);
            std::erase_if(req->headers, [phd](const avR::AvRequestParam &h) { return h.id == phd; });
            pendingHeaderDel.reset();
        }
    }
    void DetailedRequestViewUi::render_tab_body() const
    {
        if (!this->shared_state->display_request->body.has_value())
            this->shared_state->display_request->body.emplace();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::InputTextMultiline("##body", &this->shared_state->display_request->body.value(),
                                  ImVec2(avail.x, avail.y), ImGuiInputTextFlags_AllowTabInput);
    }
    void DetailedRequestViewUi::render_tab_cookies() const
    {
        avR::UiScopedStyle style;
        style.color(ImGuiCol_Button, tableXButtonColor);

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;

        if (ImGui::BeginTable("tab_cookies", 4, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("include", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("##delete", ImGuiTableColumnFlags_WidthFixed, 30);
            ImGui::TableHeadersRow();

            for (avR::AvRequestCookie &cookie : this->shared_state->display_request->cookies)
            {
                ImGui::PushID(&cookie);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText("##k", &cookie.key);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-FLT_MIN);
                avUi::InputTextAutocomplete("##v", &cookie.value, envVars);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    this->shared_state->display_request->pending_save = true;
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##inc", &cookie.included))
                    this->shared_state->display_request->pending_save = true;

                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_XMARK))
                    pendingCookieDel = cookie.id;
                ImGui::SetItemTooltip("remove cookie");
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        if (ImGui::Button(ICON_FA_PLUS "  add cookie"))
        {
            avR::AvRequest *req = this->shared_state->display_request;
            avR::AvRequestCookie row{avR::AvRequestParam{.request_id = req->id}};
            if (req->is_recent)
                row.id = next_scratch_row_id(req->cookies);
            req->cookies.push_back(row);
            req->pending_save = true;
        }

        if (pendingCookieDel.has_value())
        {
            const int64_t pcd = pendingCookieDel.value();
            avR::AvRequest *req = this->shared_state->display_request;
            // a snapshot's rows are scratch (negative ids) - there is no DB row to delete
            if (!req->is_recent)
                this->request_cookies_storage->del(pcd);
            std::erase_if(req->cookies, [pcd](const avR::AvRequestParam &c) { return c.id == pcd; });
            pendingCookieDel.reset();
        }
    }
    void DetailedRequestViewUi::render_shortcuts() const
    {
        if (!switchShortcuts)
            return;

        ImGui::OpenPopup("shortcuts");

        if (ImGui::BeginPopupModal("shortcuts", &switchShortcuts,
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration))
        {
            ImGui::TextDisabled(ICON_FA_KEYBOARD "  shortcuts");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::BeginTable("table_shortcuts", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("binding", ImGuiTableColumnFlags_WidthFixed);

            for (const UiShortcut &s : this->shared_state->shortcutManager.shortcuts)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", s.display.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::SameLine();
                ImGui::Text("%s", s.binding.c_str());
            }

            ImGui::EndTable();
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                ImGui::CloseCurrentPopup();
                switchShortcuts = false;
            }

            ImGui::Spacing();
            ImGui::TextDisabled(ICON_FA_DATABASE "  saved:");
            ImGui::SameLine();
            // the link label doubles as the path handed to the shell, so the icon stays out of it
            std::string dbPath{this->request_storage->get_db_path()};
            if (ImGui::TextLink(dbPath.c_str()))
            {
#if defined(_WIN32)
                ShellExecuteA(nullptr, "open", dbPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
                std::system(("open -R \"" + dbPath + "\"").c_str());
#else
                std::system(("xdg-open \"" + dbPath + "\"").c_str());
#endif
            }

            ImGui::EndPopup();
        }
    }
    void DetailedRequestViewUi::render_menu() const
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("actions"))
            {
                if (ImGui::MenuItem(ICON_FA_FILE_CIRCLE_PLUS "  new request", "ctrl + n"))
                {
                    this->shared_state->on_new_request.value()();
                }

                if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK "  save", "ctrl + s"))
                {
                    this->shared_state->on_save_changes.value()();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("help"))
            {
                if (ImGui::MenuItem(ICON_FA_KEYBOARD "  shortcuts", "ctrl + /"))
                {
                    this->shared_state->on_show_shortcuts.value()();
                }

                if (ImGui::MenuItem(ICON_FA_PALETTE "  style editor", "ctrl + e"))
                {
                    this->shared_state->on_show_style_editor.value()();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }
} // namespace avUi
