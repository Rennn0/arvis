#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/logo_icon.hpp>

#include <ranges>
#include <chrono>

namespace avUi
{
    RequstListViewUi::RequstListViewUi(std::string id) : avR::UiComponent(std::move(id))
    {
        this->windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        // this->style.frame_rounding = 10.f;
        // this->style.frame_padding = ImVec2(8, 8);
        // this->style.window_padding = ImVec2(20, 5);
    }

    RequstListViewUi::RequstListViewUi(std::string id, avR::AvState *sharedState) : RequstListViewUi(id)
    {
        this->shared_state = static_cast<avR::AvInterViewSharedState *>(sharedState);
        this->request_list_state = std::make_shared<avR::AvRequestListState>();
        this->request_list_state->environment = "Development";
        this->request_list_state->requests.reserve(40);
        this->request_list_state->requests.push_back(avR::AvRequest{
            .id = 1,
            .timestamp = 5000,
            .method = avNet::request_method::get,
            .title = "get products",
        });
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 2, .timestamp = 5000, .method = avNet::request_method::get, .title = "post product"});
        this->request_list_state->requests.push_back(avR::AvRequest{
            .id = 3, .timestamp = 5000, .method = avNet::request_method::get, .title = "delete product"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 4, .timestamp = 5000, .method = avNet::request_method::get, .title = "healthcheck"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 5, .timestamp = 5000, .method = avNet::request_method::get, .title = "get products"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 6, .timestamp = 5000, .method = avNet::request_method::get, .title = "post product"});
        this->request_list_state->requests.push_back(avR::AvRequest{
            .id = 7, .timestamp = 5000, .method = avNet::request_method::get, .title = "delete product"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 8, .timestamp = 5000, .method = avNet::request_method::get, .title = "healthcheck"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 9, .timestamp = 5000, .method = avNet::request_method::get, .title = "get products"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 10, .timestamp = 5000, .method = avNet::request_method::get, .title = "post product"});
        this->request_list_state->requests.push_back(avR::AvRequest{
            .id = 11, .timestamp = 5000, .method = avNet::request_method::get, .title = "delete product"});
        this->request_list_state->requests.push_back(
            avR::AvRequest{.id = 12, .timestamp = 5000, .method = avNet::request_method::get, .title = "healthcheck"});

        this->shared_state->display_request = &this->request_list_state->requests.front();
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
        ImGui::TextColored(ImColor(8, 249, 24), "%s", env);

        const char *addLabel = "+";
        const float addLabelButtonWidth = ImGui::CalcTextSize(addLabel).x + style.FramePadding.x * 3.f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - addLabelButtonWidth);
        if (ImGui::Button(addLabel))
        {
            const int64_t lastId = this->request_list_state->requests.back().id;
            using namespace std::chrono;
            this->request_list_state->requests.push_back(avR::AvRequest{
                .id = lastId + 1,
                .timestamp = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count(),
            });
            this->shared_state->display_request = &this->request_list_state->requests.back();
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
                const float itemHeight =
                    ImGui::GetTextLineHeight() * 2 + style.ItemSpacing.y + style.FramePadding.y * 2;
                const avR::AvRequest *selected = this->shared_state->display_request;

                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Indent(12.f);
                ImGui::TextDisabled("Today");
                ImGui::Unindent(12.f);
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                for (const avR::AvRequest &request :
                     this->request_list_state->requests |
                         std::views::filter([](const avR::AvRequest &r) { return r.id < 5; }))
                {
                    ImGui::PushID(request.id);
                    if (ImGui::Selectable(request.display_name().c_str(), selected && request.id == selected->id,
                                          ImGuiSelectableFlags_None, ImVec2(0, itemHeight)))
                    {
                        this->shared_state->display_request = &request;
                    }
                    ImGui::PopID();
                }

                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                ImGui::Indent(12.f);
                ImGui::TextDisabled("Yesterday");
                ImGui::Unindent(12.f);
                ImGui::Dummy(ImVec2(0.0f, 5.0f));
                for (const avR::AvRequest &request :
                     this->request_list_state->requests |
                         std::views::filter([](avR::AvRequest &r) { return r.id >= 5; }))
                {
                    ImGui::PushID(request.id);
                    if (ImGui::Selectable(request.display_name().c_str(), selected && request.id == selected->id,
                                          ImGuiSelectableFlags_None, ImVec2(0, itemHeight)))
                    {
                        this->shared_state->display_request = &request;
                    }
                    ImGui::PopID();
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
} // namespace avUi
