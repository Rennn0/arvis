#include <av_ui/root_ui.hpp>
#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/detailed_request_view_ui.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include "cousine_regular.h"
#include "roboto_medium.h"

namespace avUi
{
    RootUi::RootUi(std::string id)
        : UiComponent(id), viewport(ImGui::GetMainViewport()),
          inter_view_state(std::make_shared<avR::AvInterViewSharedState>())
    {
        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowRounding = 5.f;
        style.WindowBorderSize = 1;
        style.FrameRounding = 8.f;
        style.WindowPadding = ImVec2(3, 10);
        style.FramePadding = ImVec2(10, 6);
        style.FrameBorderSize = 1;
        style.SeparatorTextAlign = ImVec2(.5f, .5f);
        style.SeparatorTextBorderSize = 0;
        style.ScrollbarSize = 1.f;
        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.21f, 0.54f);
        colors[ImGuiCol_Tab] = ImVec4(0.17f, 0.18f, 0.20f, 0.86f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_Header] = ImVec4(0.42f, 0.51f, 0.62f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.19f, 0.20f, 0.80f);

        ImGuiIO &io = ImGui::GetIO();
        ImFontConfig cfg;
        snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "Cousine 14");
        this->FontCousine = io.Fonts->AddFontFromMemoryCompressedTTF(Font::CousineRegular_compressed_data,
                                                                     Font::CousineRegular_compressed_size, 14.f, &cfg);
        snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "Cousine 18");
        this->FontCousineLarge = io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::CousineRegular_compressed_data, Font::CousineRegular_compressed_size, 18.f, &cfg);
        snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "Roboto Medium 14");
        this->FontRoboto = io.Fonts->AddFontFromMemoryCompressedTTF(Font::RobotoMedium_compressed_data,
                                                                    Font::RobotoMedium_compressed_size, 14.f, &cfg);
        snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "Roboto Medium 18");
        this->FontRobotoLarge = io.Fonts->AddFontFromMemoryCompressedTTF(
            Font::RobotoMedium_compressed_data, Font::RobotoMedium_compressed_size, 18.f, &cfg);

        this->FontDefault = this->FontCousine;
        io.FontDefault = this->FontRobotoLarge;

        this->add_child(std::make_unique<avUi::RequstListViewUi>("req_list_view", this->inter_view_state.get()));
        this->add_child(std::make_unique<avUi::DetailedRequestViewUi>("detailed_view", this->inter_view_state.get()));
    }

    RootUi::~RootUi()
    {
        viewport = nullptr;
    }

    void RootUi::render()
    {
        for (const std::unique_ptr<UiComponent> &child : get_children())
            child->draw();
    }
} // namespace avUi
