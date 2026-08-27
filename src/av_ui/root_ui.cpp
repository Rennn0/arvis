#include <av_ui/root_ui.hpp>
#include <av_ui/request_list_view_ui.hpp>
#include <av_ui/detailed_request_view_ui.hpp>
#include <av_ui/search_view_ui.hpp>
#include <av_ui/settings_view_ui.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_ui/icons_font_awesome7.hpp>
#include "fonts/cousine_regular.h"
#include "fonts/roboto_medium.h"
#include "fonts/noto_sans_georgian.h"
#include "fonts/fa_solid_900.h"

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
        style.TabRounding = 0.f;
        style.TabBorderSize = 1.f;
        ImGuiIO &io = ImGui::GetIO();

        // Cousine and Roboto are Latin-only faces, and ImGui's default glyph range stops at
        // Latin-1 anyway — so Georgian text in a response (ა-ჰ) had no glyph to draw and came out
        // blank. A Georgian-only face is merged into every font below to fill exactly that gap.
        // The ranges array must outlive this ctor: the atlas stores the pointer and does not
        // rasterize until the first NewFrame.
        static const ImWchar georgian_ranges[] = {
            0x10A0, 0x10FF, // Georgian: Asomtavruli + Mkhedruli (the everyday alphabet)
            0x1C90, 0x1CBF, // Georgian Extended: Mtavruli
            0x2D00, 0x2D2F, // Georgian Supplement: Nuskhuri
            0,
        };

        // Font Awesome 7 Free (Solid) is merged the same way so ICON_FA_* literals draw inline with
        // text and inherit ImGuiCol_Text. ImWchar is 16-bit here (nothing defines IMGUI_USE_WCHAR32),
        // so the range has to stay inside the BMP — ICON_MAX_16_FA is exactly that bound. Same
        // lifetime rule as georgian_ranges: static, because the atlas only reads it at NewFrame.
        static const ImWchar icon_ranges[] = {
            ICON_MIN_FA,
            ICON_MAX_16_FA,
            0,
        };

        // add one font plus its Georgian and icon merges at the same pixel size. MergeMode folds a
        // face into the font added immediately before it, so all three must stay adjacent.
        const auto add_font = [&io](const char *name, const unsigned char *data, unsigned int size, float px)
        {
            ImFontConfig cfg;
            snprintf(cfg.Name, IM_ARRAYSIZE(cfg.Name), "%s", name);
            ImFont *font = io.Fonts->AddFontFromMemoryCompressedTTF(data, size, px, &cfg);

            ImFontConfig geo;
            geo.MergeMode = true;
            geo.GlyphRanges = georgian_ranges;
            snprintf(geo.Name, IM_ARRAYSIZE(geo.Name), "%s + Georgian", name);
            io.Fonts->AddFontFromMemoryCompressedTTF(Font::NotoSansGeorgian_compressed_data,
                                                     Font::NotoSansGeorgian_compressed_size, px, &geo);

            ImFontConfig icons;
            icons.MergeMode = true;
            icons.GlyphRanges = icon_ranges;
            icons.PixelSnapH = true;              // line art: pixel snapping keeps the edges crisp
            icons.GlyphMinAdvanceX = px;          // uniform icon width so icon columns line up
            icons.GlyphOffset = ImVec2(0.f, 1.f); // nudge the glyph box onto the text baseline
            snprintf(icons.Name, IM_ARRAYSIZE(icons.Name), "%s + FA7", name);
            io.Fonts->AddFontFromMemoryCompressedTTF(Font::FaSolid_compressed_data, Font::FaSolid_compressed_size, px,
                                                     &icons);
            return font;
        };

        this->FontCousine =
            add_font("Cousine 14", Font::CousineRegular_compressed_data, Font::CousineRegular_compressed_size, 14.f);
        this->FontCousineLarge =
            add_font("Cousine 18", Font::CousineRegular_compressed_data, Font::CousineRegular_compressed_size, 18.f);
        this->FontRoboto =
            add_font("Roboto Medium 14", Font::RobotoMedium_compressed_data, Font::RobotoMedium_compressed_size, 14.f);
        this->FontRobotoLarge =
            add_font("Roboto Medium 18", Font::RobotoMedium_compressed_data, Font::RobotoMedium_compressed_size, 18.f);

        this->FontDefault = this->FontCousine;
        io.FontDefault = this->FontDefault;

        avR::AvInterViewSharedState *shared = this->inter_view_state.get();
        this->add_child(std::make_unique<avUi::RequstListViewUi>("req_list_view", shared));
        this->add_child(std::make_unique<avUi::DetailedRequestViewUi>("detailed_view", shared));
        this->add_child(std::make_unique<avUi::SearchViewUi>("search_view", shared));
        this->add_child(std::make_unique<avUi::SettingsViewUi>("settings_view", shared));

        shared->shortcutManager.add(UiShortcut{ICON_FA_FILE_CIRCLE_PLUS "  New request", "ctrl + n",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_new_request.has_value();
                                               },
                                               [shared]() { shared->on_new_request.value()(); }});

        shared->shortcutManager.add(UiShortcut{ICON_FA_PAPER_PLANE "  Send request", "ctrl + enter",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Enter,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_send_request.has_value();
                                               },
                                               [shared]() { shared->on_send_request.value()(); }});

        shared->shortcutManager.add(UiShortcut{ICON_FA_MAGNIFYING_GLASS "  Search", "ctrl + f",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_show_search.has_value();
                                               },
                                               [shared]() { shared->on_show_search.value()(); }});

        shared->shortcutManager.add(UiShortcut{ICON_FA_FLOPPY_DISK "  Save changes", "ctrl + s",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_save_changes.has_value();
                                               },
                                               [shared]() { shared->on_save_changes.value()(); }});

        shared->shortcutManager.add(UiShortcut{ICON_FA_KEYBOARD "  Show shortcuts", "ctrl + /",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Slash,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_show_shortcuts.has_value();
                                               },
                                               [shared]() { shared->on_show_shortcuts.value()(); }});

        shared->shortcutManager.add(UiShortcut{ICON_FA_PALETTE "  Show style editor", "ctrl + e",
                                               [shared]()
                                               {
                                                   return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_E,
                                                                          ImGuiInputFlags_RouteGlobal) &&
                                                          shared->on_show_style_editor.has_value();
                                               },
                                               [shared]() { shared->on_show_style_editor.value()(); }});

        shared->shortcutManager.add(UiShortcut{
            ICON_FA_GEAR "  Show settings", "ctrl + shift + p",
            [shared]()
            {
                return ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, ImGuiInputFlags_RouteGlobal) &&
                       shared->on_show_settings.has_value();
            },
            [shared]() { shared->on_show_settings.value()(static_cast<size_t>(avUi::Section::General)); }});
    }

    RootUi::~RootUi()
    {
        viewport = nullptr;
    }

    void RootUi::render()
    {
        for (const std::unique_ptr<UiComponent> &child : get_children())
        {
            child->update();
            child->draw();
        }

        this->inter_view_state->shortcutManager.process();
    }

} // namespace avUi
