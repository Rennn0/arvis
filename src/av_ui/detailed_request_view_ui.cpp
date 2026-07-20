#include <av_ui/detailed_request_view_ui.hpp>

namespace avUi
{
    DetailedRequestViewUi::DetailedRequestViewUi(std::string id) : avR::UiComponent(std::move(id)), footer_height(-1.f)
    {
        this->window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    }

    DetailedRequestViewUi::DetailedRequestViewUi(std::string id, avR::AvState *sharedState) : DetailedRequestViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
    }

    DetailedRequestViewUi::~DetailedRequestViewUi()
    {
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

            ImGui::ShowDemoWindow();
        }
        ImGui::End();
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
                    req.method = m;
                ImGui::PopStyleColor();
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.f);
        ImGui::InputText("##title_edit", &req.url,
                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        ImGui::SameLine();
        const char *send_label = "Send";
        if (ImGui::Button(send_label))
        {
        }
    }
    void DetailedRequestViewUi::render_main_content(const ImGuiStyle &style)
    {
    }
    void DetailedRequestViewUi::render_footer(const ImGuiStyle &style)
    {
    }
} // namespace avUi
