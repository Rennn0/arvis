#pragma once
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <vector>
#include <utility>
#include <string>
#include <boost/container/small_vector.hpp>
//
// InputText with {{variable}} autocomplete.
//
//   static const av::EnvVars kVars = {
//       { "key",  "X-Api-Key" },
//       { "user", "luka" },
//       { "host", "https://api.staging.internal:8443" },
//   };
//   avUi::InputTextAutocomplete("##k", &header->key, kVars);
//
// Typing "{{" opens the list, "{{k" filters it. Tab / Enter / click accepts.
// The popup shows each variable's current value greyed out next to its name.
namespace avUi
{
    using Var = std::pair<std::string, std::string>;
    using EnvVars = std::vector<Var>;

    namespace internal
    {
        inline const std::string &var_key(const Var &v)
        {
            return v.first;
        }
        inline const std::string &var_value(const Var &v)
        {
            return v.second;
        }

        using MatchList = boost::container::small_vector<const Var *, 32>;

        struct VarCompleteState
        {
            ImGuiID owner = 0;     // InputText we're currently attached to
            ImGuiID pendingId = 0; // written by the caller just before InputText()
            bool open = false;
            bool dismissed = false;
            bool scrollToSel = false;
            bool commit = false;
            bool refocus = false;
            int sel = 0;
            int tokenStart = -1;
            int cursorPos = 0;
            std::string filter;
            std::string prevFilter;
            MatchList matches;
            const EnvVars *vars = nullptr;
        };

        VarCompleteState &state();
        void draw_popup(VarCompleteState &state, std::string *str, const ImVec2 &recMin, const ImVec2 &recMax);
        bool scan_token(const char *buf, int cursor, VarCompleteState &state);
        std::string make_insert(const std::string &name);
        std::string value_preview(const std::string &value, size_t maxLen = 48);
        void apply_in_callback(ImGuiInputTextCallbackData *d, VarCompleteState &state);
        int callback(ImGuiInputTextCallbackData *d);
        const Var *find_var(const std::string &name, const EnvVars &vars);
    } // namespace internal

    // Drop-in replacement for ImGui::InputText(label, std::string*).
    bool InputTextAutocomplete(const char *label, std::string *str, const EnvVars &variables,
                               ImGuiInputTextFlags flags = 0);
    std::string resolve_vars(const std::string &text, const EnvVars &vars, std::vector<std::string> *missing = nullptr);
} // namespace avUi