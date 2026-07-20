#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/logo_icon.hpp>

#include <ranges>
#include <chrono>

namespace avUi
{
    RequstListViewUi::RequstListViewUi(std::string id)
        : avR::UiComponent(std::move(id)), request_list_state(std::make_shared<avR::AvRequestListState>()),
          request_storage(std::make_unique<avS::AvRequestStorage>())
    {
        this->windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        // this->style.frame_rounding = 10.f;
        // this->style.frame_padding = ImVec2(8, 8);
        // this->style.window_padding = ImVec2(20, 5);
    }

    RequstListViewUi::RequstListViewUi(std::string id, avR::AvState *sharedState) : RequstListViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
        this->request_list_state->requests = this->request_storage->select_all();
        this->request_list_state->environment = "Development"; // #TODO roca env sys aewyoba esec sheicvleba

        if (this->request_list_state->requests.size() > 0)
            this->shared_state->display_request = this->request_list_state->requests.front().get();
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
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(x * .2f, y), ImGuiCond_Once);

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
    void RequstListViewUi::render_header(const ImGuiStyle &style)
    {
        const float marginTop = 10.f;
        const float marginLeft = 15.f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + marginTop);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + marginLeft);

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("env:");
        ImGui::SameLine();
        const char *env = this->request_list_state->environment.c_str();
        ImGui::TextColored(this->environment_color, "%s", env);

        const char *addLabel = "+";
        const float addLabelButtonWidth = ImGui::CalcTextSize(addLabel).x + style.FramePadding.x * 3.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - addLabelButtonWidth);
        if (ImGui::Button(addLabel))
        {
            using namespace std::chrono;
            std::shared_ptr<avR::AvRequest> req = std::make_shared<avR::AvRequest>(avR::AvRequest{
                .timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(),
            });
            this->request_list_state->requests.push_back(std::move(req));
            this->request_storage->upsert(this->request_list_state->requests);
            this->shared_state->display_request = this->request_list_state->requests.back().get();
        }
        ImGui::SetItemTooltip("add request");

        ImGui::Dummy(ImVec2(0.f, marginTop));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + marginLeft);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - marginLeft);
        ImGui::InputTextWithHint("##filter", "filter requsts", &this->filter_text);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            this->log_info(this->filter_text);
        }
    }
    void RequstListViewUi::render_main_content(const ImGuiStyle &style)
    {
        if (ImGui::BeginTabBar("request_history_saved"))
        {
            if (ImGui::BeginTabItem("history"))
            {
                const avR::AvRequest *selected = this->shared_state->display_request;

                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Indent(12.f);
                ImGui::TextDisabled("Today");
                ImGui::Unindent(12.f);
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                for (std::shared_ptr<avR::AvRequest> &request :
                     this->request_list_state->requests |
                         std::views::filter([](const std::shared_ptr<avR::AvRequest> &r) { return r->id < 5; }))
                {
                    this->render_request_row(selected, request.get(), style);
                }
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Indent(12.f);
                ImGui::TextDisabled("Yesterday");
                ImGui::Unindent(12.f);
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                for (std::shared_ptr<avR::AvRequest> &request :
                     this->request_list_state->requests |
                         std::views::filter([](const std::shared_ptr<avR::AvRequest> &r) { return r->id >= 5; }))
                {
                    this->render_request_row(selected, request.get(), style);
                }

                if (this->pending_delete_req.has_value())
                {
                    const int64_t id = this->pending_delete_req.value();
                    if (this->shared_state->display_request && this->shared_state->display_request->id == id)
                        this->shared_state->display_request = nullptr;

                    this->request_storage->del(id);
                    std::erase_if(this->request_list_state->requests,
                                  [id](std::shared_ptr<avR::AvRequest> x) { return x->id == id; });

                    this->pending_delete_req.reset();
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("saved"))
            {

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    void RequstListViewUi::render_footer(const ImGuiStyle &style)
    {
        const float savedTxtOffset = 15.f;
        const int savedReqCountMock = 102;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + savedTxtOffset);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%d saved", savedReqCountMock);

        const char *envLabel = "env";
        const float envLabelButtonWidth = ImGui::CalcTextSize(envLabel).x + style.FramePadding.x * 3.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - envLabelButtonWidth);

        if (ImGui::Button(envLabel))
        {
        }
        ImGui::SetItemTooltip("modify environment variables");
    }

    void RequstListViewUi::render_request_row(const avR::AvRequest *selected, avR::AvRequest *request,
                                              const ImGuiStyle &style)
    {
        ImGui::PushID(request->id);

        const bool is_selected = selected && request->id == selected->id;
        const ImVec2 row_start = ImGui::GetCursorPos();
        const float row_width = ImGui::GetContentRegionAvail().x;
        const float pad_x = style.ItemSpacing.x;
        const float line_h = ImGui::GetTextLineHeight();

        // --- line 1 column geometry: [METHOD]  [wrapped title]  [STATUS] ---
        const char *method_txt = avNet::NetworkManager::method_text(request->method);
        const float method_w = ImGui::CalcTextSize(method_txt).x;

        char status_buf[8];
        std::snprintf(status_buf, sizeof(status_buf), "%d", request->status_code.value_or(0));
        const float status_w = ImGui::CalcTextSize(status_buf).x;

        const std::string title_txt = request->display_name(); // copy: unambiguous lifetime

        const float title_x = row_start.x + pad_x + method_w + style.ItemSpacing.x;
        const float status_x = row_start.x + row_width - status_w - pad_x;
        const float title_right = status_x - style.ItemSpacing.x;             // absolute wrap X
        const float title_wrap_width = std::max(title_right - title_x, 1.0f); // matching width
        const float title_h =
            std::max(ImGui::CalcTextSize(title_txt.c_str(), nullptr, false, title_wrap_width).y, line_h);

        // --- url geometry (line 2+) ---
        const float wrap_width = row_width - pad_x * 2.0f;
        const float url_h = ImGui::CalcTextSize(request->url.c_str(), nullptr, false, wrap_width).y;

        // line 1 is as tall as the (possibly multi-line) title
        const float row_height = style.FramePadding.y * 2.0f + title_h + style.ItemSpacing.y + url_h;

        // 1) clickable box, dynamic height
        if (ImGui::Selectable("##row", is_selected, ImGuiSelectableFlags_None, ImVec2(0.0f, row_height)))
        {
            this->shared_state->display_request = request;
        }
        const ImVec2 row_end = ImGui::GetCursorPos();

        // 2) right-click -> title edit panel (must stay right after the selectable)
        if (ImGui::BeginPopupContextItem())
        {
            // ImGui::SetNextItemWidth(200.0f);
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            if (!request->title.has_value())
                request->title.emplace(request->display_name());
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("title");
            ImGui::SameLine();
            if (ImGui::InputText("##title", &request->title.value(), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                this->request_storage->upsert(request);
                ImGui::CloseCurrentPopup();
            }
            ImGui::Spacing();
            if (ImGui::Button("delete"))
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
        const float line1_y = row_start.y + style.FramePadding.y;

        ImGui::SetCursorPos(ImVec2(row_start.x + pad_x, line1_y));
        ImGui::TextColored(this->get_method_color(request->method), "%s", method_txt);

        ImGui::SetCursorPos(ImVec2(status_x, line1_y));
        ImGui::TextColored(this->get_status_color(request->status_code.value_or(0)), "%s", status_buf);

        ImGui::SetCursorPos(ImVec2(title_x, line1_y));
        ImGui::PushTextWrapPos(title_right);
        ImGui::TextUnformatted(title_txt.c_str());
        ImGui::PopTextWrapPos();

        // 4) line 2+: wrapped url
        ImGui::SetCursorPos(ImVec2(row_start.x + pad_x, line1_y + title_h + style.ItemSpacing.y));
        ImGui::PushTextWrapPos(row_start.x + row_width - pad_x);
        ImGui::TextDisabled("%s", request->url.c_str());
        ImGui::PopTextWrapPos();

        // 5) restore layout cursor
        ImGui::SetCursorPos(row_end);
        ImGui::PopID();
    }
} // namespace avUi
