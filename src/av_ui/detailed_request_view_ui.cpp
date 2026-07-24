#include <av_ui/detailed_request_view_ui.hpp>
#include <av_ui/json_tree_view.hpp>

namespace avUi
{
    std::optional<int64_t> pendingParamDel;
    std::optional<int64_t> pendingHeaderDel;
    std::optional<int64_t> capturedReqId;
    bool showStyles = false;
    bool showShortcuts = false;

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id)
        : avR::UiComponent(std::move(id)), footer_height(-1.f),
          request_storage(std::make_unique<avS::AvRequestStorage>()),
          request_params_storage(std::make_unique<avS::AvRequestParamsStorage>()),
          request_headers_storage(std::make_unique<avS::AvRequestHeadersStorage>()),
          json_view(std::make_unique<JsonTreeView>())
    {
        this->window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize;
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id, avR::AvState *sharedState) : DetailedRequestViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);

        this->shared_state->shortcut.add(UiShortcut{"New request", "ctrl + n",
                                                    []() { return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N); },
                                                    [this]()
                                                    {
                                                        if (this->shared_state->on_new_request)
                                                            this->shared_state->on_new_request();
                                                    }});

        this->shared_state->shortcut.add(UiShortcut{"Send request", "ctrl + enter",
                                                    []() { return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Enter); },
                                                    [this]() { this->send_request(); }});

        this->shared_state->shortcut.add(UiShortcut{
            "Save changes", "ctrl + s",
            [this]()
            {
                return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S) && this->shared_state->display_request->pending_save;
            },
            [this]()
            {
                this->request_storage->upsert(this->shared_state->display_request);
                this->request_params_storage->upsert(this->shared_state->display_request->params);
                this->request_headers_storage->upsert(this->shared_state->display_request->headers);
                this->shared_state->display_request->pending_save = false;
            }});

        this->shared_state->shortcut.add(UiShortcut{"Show shortcuts", "ctrl + /",
                                                    []() { return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Slash); },
                                                    [&]() { showShortcuts = true; }});
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
        // never let the worker outlive this component (and its NetworkManager / response_body).
        if (this->pending_response.valid())
            this->pending_response.wait();
    }

    void DetailedRequestViewUi::render()
    {
        if (this->shared_state && !this->shared_state->show_req_detailed_view)
            return;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const float x = viewport->WorkSize.x;
        const float y = viewport->WorkSize.y;
        ImGui::SetNextWindowPos(ImVec2(x * .2f, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(x * .8f, y), ImGuiCond_Once);

        if (ImGui::Begin(this->get_id().c_str(), &this->shared_state->show_req_detailed_view, this->window_flags))
        {
            this->shared_state->shortcut.process();

            if (!this->shared_state->display_request)
            {
                const char *msg = "No request selected";
                const ImVec2 avail = ImGui::GetContentRegionAvail();
                const ImVec2 sz = ImGui::CalcTextSize(msg);
                ImGui::SetCursorPos(ImVec2((avail.x - sz.x) * .5f, (avail.y - sz.y) * .5f));
                ImGui::TextDisabled("%s", msg);
                ImGui::End();
                return;
            }

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("file"))
                {
                    if (ImGui::MenuItem("new request"))
                    {
                    }

                    if (ImGui::MenuItem("save"))
                    {
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("help"))
                {
                    if (ImGui::MenuItem("shortcuts", "ctrl + /"))
                    {
                        showShortcuts = true;
                    }

                    if (ImGui::MenuItem("style editor"))
                    {
                        showStyles = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // pick up a completed request once per frame so the header (button state) and
            // footer (response) agree within the same frame.
            this->poll_response();

            const ImGuiStyle &style = ImGui::GetStyle();
            const ImVec2 availRegion = ImGui::GetContentRegionAvail();

            if (this->footer_height < 0.f)
                this->footer_height = availRegion.y * .4f;

            const float splitterThikness = 3.f;
            const float minFooter = 50.f;
            const float maxFooter = availRegion.y - 100.f;
            this->footer_height = std::clamp(this->footer_height, minFooter, maxFooter);
            const float headerHeight = (availRegion.y - this->footer_height) * .1f;

            ImGui::BeginChild("header", ImVec2(0, headerHeight));
            this->render_header(style);
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::BeginChild("main_content", ImVec2(0, -(this->footer_height + splitterThikness)));
            this->render_main_content(style);
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
            this->render_footer(style);
            ImGui::EndChild();

            if (showShortcuts)
                ImGui::OpenPopup("popup_shortcuts");

            if (ImGui::BeginPopupModal("popup_shortcuts", &showShortcuts, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::BeginTable("table_shortcuts", 2);

                for (const UiShortcut &s : this->shared_state->shortcut.shortcuts)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(s.display.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SameLine();
                    ImGui::Text(s.binding.c_str());
                }

                ImGui::EndTable();
                if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    ImGui::CloseCurrentPopup();
                    showShortcuts = false;
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();

        if (showStyles)
        {
            if (ImGui::Begin("style editor", &showStyles))
            {
                ImGui::ShowStyleEditor();
            }

            if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                showStyles = false;
            }
            ImGui::End();
        }
    }
    void DetailedRequestViewUi::render_header(const ImGuiStyle &style)
    {
        avR::AvRequest &req = *this->shared_state->display_request;
        if (!req.title.has_value())
            req.title.emplace(req.display_name());

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
        if (ImGui::InputText("##title_edit", &req.url,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            this->save_state_change();
        };

        ImGui::SameLine();
        const bool in_flight = this->pending_response.valid();
        ImGui::BeginDisabled(in_flight);
        if (ImGui::Button(in_flight ? "Sending..." : "Send"))
        {
            this->send_request();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", this->root.timestamp_to_date(req.timestamp).c_str());

        if (this->shared_state->display_request->pending_save)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted("*");
            ImGui::SetItemTooltip("press ctrl+s to save");
        }
    }
    void DetailedRequestViewUi::render_main_content(const ImGuiStyle &style)
    {
        if (ImGui::BeginTabBar("req_tabs"))
        {
            if (!capturedReqId.has_value() || capturedReqId.value() != this->shared_state->display_request->id)
            {
                capturedReqId = this->shared_state->display_request->id;
                this->shared_state->display_request->params =
                    this->request_params_storage->select_by_req_id(capturedReqId.value());

                this->shared_state->display_request->headers =
                    this->request_headers_storage->select_by_req_id(capturedReqId.value());
            }

            if (ImGui::BeginTabItem("params"))
            {
                this->render_tab_params();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("headers"))
            {
                this->render_tab_headers();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("body"))
            {
                this->render_tab_body();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("auth"))
            {
                this->render_tab_auth();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
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

        if (req.body && !req.body->empty())
        {
            out += " --data ";
            out += shell_single_quote(*req.body);
        }

        return out;
    }

    void DetailedRequestViewUi::render_footer(const ImGuiStyle &style)
    {
        if (this->pending_response.valid())
        {
            ImGui::TextDisabled("Sending request...");
            return;
        }

        if (this->shared_state->display_request->last_response_body.empty() &&
            this->shared_state->display_request->last_response_http_code == 0)
        {
            ImGui::TextDisabled("No response yet - press Send to run the request.");
            return;
        }

        const bool ok = this->shared_state->display_request->last_status == avNet::response_status::Ok;
        const ImVec4 statusColor = ok ? ImVec4(0.40f, 0.80f, 0.40f, 1.f) : ImVec4(0.90f, 0.40f, 0.40f, 1.f);

        this->shared_state->display_request->status_code = this->shared_state->display_request->last_response_http_code;

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(statusColor, "%s",
                           avNet::NetworkManager::status_text(this->shared_state->display_request->last_status));
        ImGui::SameLine();
        ImGui::TextDisabled("HTTP %ld", this->shared_state->display_request->last_response_http_code);
        ImGui::SameLine();
        ImGui::TextDisabled("(%zu bytes)", this->shared_state->display_request->last_response_body.size());

        // copy options sit next to the status line and act on the request we actually sent
        // (last_request) and the response we got back (response_body).
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        if (ImGui::SmallButton("copy"))
            ImGui::OpenPopup("##copy_options");
        if (ImGui::BeginPopup("##copy_options"))
        {
            if (ImGui::Selectable("copy req as curl"))
                ImGui::SetClipboardText(format_as_curl(this->shared_state->display_request->last_request).c_str());
            if (ImGui::Selectable("copy raw response"))
                ImGui::SetClipboardText(this->shared_state->display_request->last_response_body.c_str());
            if (ImGui::Selectable("copy request url"))
                ImGui::SetClipboardText(this->shared_state->display_request->last_request.url.c_str());
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // JSON bodies get the interactive tree / pretty views; anything else (HTML, plain text,
        // an error string) falls back to the raw text view. The selector only appears when there
        // is a tree/pretty view to switch to.
        const bool json = this->json_view->is_json();
        if (json)
        {
            const auto mode_tab = [&](const char *label, ResponseView mode)
            {
                const bool active = this->response_view == mode;
                if (active)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                if (ImGui::SmallButton(label))
                    this->response_view = mode;
                if (active)
                    ImGui::PopStyleColor();
            };
            mode_tab("Tree", ResponseView::tree);
            ImGui::SameLine();
            mode_tab("Pretty", ResponseView::pretty);
            ImGui::SameLine();
            mode_tab("Raw", ResponseView::raw);
        }

        if (json && this->response_view == ResponseView::tree)
        {
            this->json_view->render_tree();
        }
        else if (json && this->response_view == ResponseView::pretty)
        {
            this->json_view->render_pretty();
        }
        else
        {
            // read-only body view fills the remaining footer space. It wraps at the child's right
            // edge (no horizontal scroll) and only scrolls vertically on overflow.
            if (ImGui::BeginChild("##response_body_view", ImVec2(0, 0)))
            {
                ImGui::PushTextWrapPos(0.f); // 0 == wrap at the child's right edge
                ImGui::TextUnformatted(this->shared_state->display_request->last_response_body.data(),
                                       this->shared_state->display_request->last_response_body.data() +
                                           this->shared_state->display_request->last_response_body.size());
                ImGui::PopTextWrapPos();
            }
            ImGui::EndChild();
        }
    }

    void DetailedRequestViewUi::send_request()
    {
        if (!this->shared_state || !this->shared_state->display_request)
            return;

        // one request at a time; ignore the button while a fetch is in flight.
        if (this->pending_response.valid())
            return;

        avR::AvRequest *req = this->shared_state->display_request;

        // params are the source of truth for the query string: assemble the final url from
        // them, reflect it back into the editable url, and persist.
        std::string url = build_url(req->url, req->params);
        req->url = url;
        this->save_state_change();

        // snapshot the request (method + url + included headers + body) into a self-contained
        // descriptor the worker owns. It copies the header strings, so the worker never races
        // against edits to display_request on the UI thread while the fetch is in flight.
        avNet::http_request request;
        request.method = req->method;
        request.url = std::move(url);
        request.body = req->body;
        for (const avR::AvRequestHeader &header : req->headers)
        {
            if (!header.included || header.key.empty())
                continue;
            request.headers.emplace_back(header.key, header.value);
        }

        // keep an independent copy of exactly what we send so the footer can reproduce it
        // (copy as cURL, etc.) without touching the worker's moved-in copy.
        this->shared_state->display_request->last_request = request;

        // reset the destination on the UI thread before the worker starts writing to it.
        // std::launch::async establishes a happens-before with the worker; the UI thread only
        // reads response_body / response_http_code again after the future is ready (poll_response).
        this->shared_state->display_request->last_response_body.clear();
        this->shared_state->display_request->last_response_http_code = 0;

        // run off the UI thread so a slow/dead endpoint never freezes the window.
        this->pending_response = std::async(std::launch::async,
                                            [this, request = std::move(request)]()
                                            {
                                                return this->network_manager.send(
                                                    request, this->shared_state->display_request->last_response_body,
                                                    &this->shared_state->display_request->last_response_http_code);
                                            });
    }

    void DetailedRequestViewUi::poll_response()
    {
        if (this->pending_response.valid() &&
            this->pending_response.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            this->shared_state->display_request->last_status = this->pending_response.get();

            // parse the body once, here, rather than every frame in the footer. Default to the
            // tree view when it is JSON, otherwise fall back to the raw text view.
            this->json_view->set_source(this->shared_state->display_request->last_response_body);
            this->response_view = this->json_view->is_json() ? ResponseView::tree : ResponseView::raw;
        }
    }

    void DetailedRequestViewUi::save_state_change() const
    {
        this->request_storage->upsert(this->shared_state->display_request);
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
            url += url_encode(p.value);
        }

        url.append(fragment);
        return url;
    }
    void DetailedRequestViewUi::render_tab_params() const
    {
        avR::UiScopedStyle style(avR::UiScopedStyle::Style{.frame_rounding = 0, .frame_border = 0});

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;

        if (ImGui::BeginTable("params_table", 5, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("include");
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            const size_t count = this->shared_state->display_request->params.size();
            clipper.Begin(count);
            while (clipper.Step())
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    avR::AvRequestParam *item = &this->shared_state->display_request->params[row_n];
                    ImGui::PushID(item);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##key", &item->key);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        this->shared_state->display_request->pending_save = true;
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##val", &item->value);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        this->shared_state->display_request->pending_save = true;

                    ImGui::TableNextColumn();
                    if (item->editing)
                    {
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (item->set_focus)
                        {
                            ImGui::SetKeyboardFocusHere();
                            item->set_focus = false;
                        }
                        ImGui::InputText("##desc", &item->description);
                        if (ImGui::IsItemDeactivated())
                        {
                            item->editing = false;
                            this->shared_state->display_request->pending_save = true;
                        }
                    }
                    else
                    {
                        ImVec2 start = ImGui::GetCursorPos();
                        float wrap = ImGui::GetContentRegionAvail().x;

                        ImGui::PushTextWrapPos(start.x + wrap);
                        ImGui::TextUnformatted(item->description.empty() ? " " : item->description.c_str());
                        ImGui::PopTextWrapPos();

                        float h = ImGui::GetItemRectSize().y;

                        ImGui::SetCursorPos(start);
                        if (ImGui::InvisibleButton("##desc_click", ImVec2(wrap, h)))
                        {
                            item->editing = true;
                            item->set_focus = true;
                        }
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##included", &item->included))
                        this->shared_state->display_request->pending_save = true;
                    ImGui::TableNextColumn();
                    if (ImGui::Button("delete"))
                        pendingParamDel = item->id;
                    ImGui::PopID();
                }
            ImGui::EndTable();
        }

        if (ImGui::Button("add param"))
        {
            this->shared_state->display_request->params.push_back(
                avR::AvRequestParam{.request_id = this->shared_state->display_request->id});
            this->shared_state->display_request->pending_save = true;
        }

        if (pendingParamDel.has_value())
        {
            const int64_t id = pendingParamDel.value();
            this->request_params_storage->del(id);
            std::erase_if(this->shared_state->display_request->params,
                          [id](avR::AvRequestParam &p) { return p.id == id; });
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

        // if (this->shared_state->pending_save)
        // {
        //     this->request_params_storage->upsert(this->shared_state->display_request->params);
        //     this->shared_state->pending_save = false;
        // }
    }
    void DetailedRequestViewUi::render_tab_headers() const
    {
        avR::UiScopedStyle style(avR::UiScopedStyle::Style{.frame_rounding = 0, .frame_border = 0});

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                                ImGuiTableFlags_SizingStretchSame;

        if (ImGui::BeginTable("tab_headers", 4, flags, ImVec2(0, 0), 0.f))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("include", ImGuiTableColumnFlags_WidthStretch);
            // ImGui::TableSetupColumn("delete", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            const size_t count = this->shared_state->display_request->headers.size();
            clipper.Begin(count);
            while (clipper.Step())
            {
                for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                {
                    avR::AvRequestHeader *header = &this->shared_state->display_request->headers[row_n];
                    ImGui::PushID(header);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##k", &header->key);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        this->shared_state->display_request->pending_save = true;
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    ImGui::InputText("##v", &header->value);
                    if (ImGui::IsItemDeactivatedAfterEdit())
                        this->shared_state->display_request->pending_save = true;
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox("##inc", &header->included))
                        this->shared_state->display_request->pending_save = true;

                    ImGui::TableNextColumn();
                    if (ImGui::Button("delete"))
                        pendingHeaderDel = header->id;
                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }

        if (ImGui::Button("add header"))
        {
            this->shared_state->display_request->headers.push_back(
                avR::AvRequestHeader{avR::AvRequestParam{.request_id = this->shared_state->display_request->id}});
            this->shared_state->display_request->pending_save = true;
        }

        // if (this->shared_state->pending_save = true)
        // {
        //     this->request_headers_storage->upsert(this->shared_state->display_request->headers);
        //     this->shared_state->pending_save = false;
        // }
    }
    void DetailedRequestViewUi::render_tab_body() const
    {
        if (!this->shared_state->display_request->body.has_value())
            this->shared_state->display_request->body.emplace();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::InputTextMultiline("##body", &this->shared_state->display_request->body.value(),
                                  ImVec2(avail.x, avail.y), ImGuiInputTextFlags_AllowTabInput);
    }
    void DetailedRequestViewUi::render_tab_auth() const
    {
    }
} // namespace avUi
