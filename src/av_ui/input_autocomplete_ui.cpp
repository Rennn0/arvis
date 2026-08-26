#include <av_ui/input_autocomplete_ui.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/algorithm/find_if.hpp>

namespace avUi
{
    bool InputTextAutocomplete(const char *label, std::string *str, const EnvVars &variables, ImGuiInputTextFlags flags)
    {
        internal::VarCompleteState &state = internal::state();
        const ImGuiID id = ImGui::GetID(label);
        state.pendingId = id;
        state.vars = &variables;
        if (state.owner == id && state.refocus)
        {
            ImGui::SetKeyboardFocusHere();
            state.refocus = false;
        }

        flags |= ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCompletion |
                 ImGuiInputTextFlags_CallbackHistory;
        const bool changed = ImGui::InputText(label, str, flags, &internal::callback, &state);
        const std::string_view tooltip = *str;
        ImGui::SetItemTooltip(resolve_vars(tooltip.data(), variables).c_str());
        const ImVec2 itemMin = ImGui::GetItemRectMin();
        const ImVec2 itemMax = ImGui::GetItemRectMax();
        const bool active = ImGui::IsItemActive();
        if (state.owner == id && state.open && active)
            internal::draw_popup(state, str, itemMin, itemMax);
        else if (state.owner == id && !active && !state.refocus)
            state.open = false;
        return changed;
    }

    std::string resolve_vars(const std::string &text, const EnvVars &vars, std::vector<std::string> *missing)
    {
        if (text.find("{{") == std::string::npos)
            return text;

        std::string out;
        out.reserve(text.size() + 32);

        size_t pos = 0;
        while (pos < text.size())
        {
            const size_t open = text.find("{{", pos);
            if (open == std::string::npos)
            {
                out.append(text, pos);
                break;
            }
            const size_t close = text.find("}}", open + 2);
            if (close == std::string::npos)
            {
                out.append(text, pos); // unterminated - emit the rest as literal
                break;
            }

            out.append(text, pos, open - pos);

            const std::string name = boost::trim_copy(text.substr(open + 2, close - open - 2));
            if (const Var *v = internal::find_var(name, vars))
            {
                out += internal::var_value(*v);
            }
            else
            {
                out.append(text, open, close + 2 - open); // leave {{name}} visible
                if (missing)
                    missing->push_back(name);
            }

            pos = close + 2;
        }
        return out;
    }

    namespace internal
    {

        VarCompleteState &state()
        {
            static VarCompleteState s;
            return s;
        }

        void draw_popup(VarCompleteState &state, std::string *str, const ImVec2 &recMin, const ImVec2 &recMax)
        {
            using namespace ImGui;
            const ImGuiStyle &style = GetStyle();
            const float lineH = GetTextLineHeightWithSpacing();
            const int count = static_cast<int>(state.matches.size());
            // Pass 1: build the value previews and measure the two columns, so the
            // window can be sized exactly instead of popping wider on the next frame.
            boost::container::small_vector<std::string, 32> previews;
            previews.reserve(count);
            float keyW = 0.f;
            float valW = 0.f;
            for (const Var *v : state.matches)
            {
                previews.push_back(value_preview(var_value(*v)));
                keyW = std::max(keyW, CalcTextSize(var_key(*v).c_str()).x);
                valW = std::max(valW, CalcTextSize(previews.back().c_str()).x);
            }
            const float gap = style.ItemSpacing.x * 3.f;
            const float keyColW = keyW + gap;
            const float padding = style.WindowPadding.x * 2.f + style.ScrollbarSize;
            const float width = std::max(recMax.x - recMin.x, keyColW + valW + padding);
            SetNextWindowPos(ImVec2(recMin.x, recMax.y + 2.f));
            SetNextWindowSizeConstraints(ImVec2(width, 0.f), ImVec2(width, lineH * 8 + style.WindowPadding.y * 2));
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 4.f));
            const ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Tooltip; // keeps it on the topmost layer
            if (Begin("##autocomplete", nullptr, flags))
            {
                for (size_t i = 0; i < count; i++)
                {
                    const Var &var = *state.matches[i];
                    const std::string &name = var_key(var);
                    PushID(i);
                    if (Selectable(name.c_str(), i == state.sel))
                    {
                        state.sel = i;
                        if (state.tokenStart >= 0 && state.cursorPos <= static_cast<int>(str->size()))
                        {
                            int end = state.cursorPos;
                            if (static_cast<int>(str->size()) >= end + 2 && (*str)[end] == '}' &&
                                (*str)[end + 1] == '}')
                            {
                                end += 2;
                            }
                            str->replace(state.tokenStart, end - state.tokenStart, make_insert(name));
                        }
                        state.open = false;
                        state.tokenStart = -1;
                        state.refocus = true;
                    }

                    if (!previews[i].empty())
                    {
                        SameLine(keyColW);
                        TextDisabled("%s", previews[i].c_str());
                    }
                    if (i == state.sel && state.scrollToSel)
                        SetScrollHereY(.5f);
                    PopID();
                }
                state.scrollToSel = false;
            }
            End();
            PopStyleVar();
        }

        bool scan_token(const char *buf, int cursor, VarCompleteState &state)
        {
            state.tokenStart = -1;
            state.filter.clear();
            state.matches.clear();
            state.cursorPos = cursor;

            const int kMaxScan = 128;
            const int stop = cursor - kMaxScan < 0 ? 0 : cursor - kMaxScan;

            const auto window = boost::make_iterator_range(buf + stop, buf + cursor);
            const auto brace = boost::algorithm::find_last(window, "{{");
            if (brace.empty())
                return false;

            state.tokenStart = static_cast<int>(brace.begin() - buf);
            state.filter.assign(brace.end(), buf + cursor);

            // Anything weird between the braces and the cursor means we're not
            // completing a name (already closed, whitespace, nested brace, ...).
            if (boost::algorithm::any_of(state.filter, boost::is_any_of("{}\n\t ")))
            {
                state.tokenStart = -1;
                state.filter.clear();
                return false;
            }

            const std::string &prefix = state.filter;
            boost::push_back(state.matches,
                             *state.vars |
                                 boost::adaptors::filtered([&prefix](const Var &v)
                                                           { return boost::istarts_with(var_key(v), prefix); }) |
                                 boost::adaptors::transformed([](const Var &v) -> const Var * { return &v; }));

            return !state.matches.empty();
        }
        std::string make_insert(const std::string &name)
        {
            return "{{" + name + "}}";
        }
        std::string value_preview(const std::string &value, size_t maxLen)
        {
            std::string s = boost::algorithm::trim_copy(value);
            boost::algorithm::replace_all(s, "\n", " ");
            boost::algorithm::replace_all(s, "\t", " ");
            if (s.size() > maxLen)
            {
                s.resize(maxLen - 3);
                s += "...";
            }
            return s;
        }
        void apply_in_callback(ImGuiInputTextCallbackData *d, VarCompleteState &state)
        {
            if (state.tokenStart < 0 || state.matches.empty())
                return;
            const int sel = boost::algorithm::clamp(state.sel, 0, state.matches.size() - 1);
            const std::string ins = make_insert(var_key(*state.matches[sel]));

            int end = d->CursorPos;
            if (d->BufTextLen >= end + 2 && d->Buf[end] == '}' && d->Buf[end + 1] == '}')
                end += 2;

            d->DeleteChars(state.tokenStart, end - state.tokenStart);
            d->InsertChars(state.tokenStart, ins.c_str());
            d->CursorPos = state.tokenStart + ins.size();
            d->SelectionStart = d->SelectionEnd = d->CursorPos;

            state.open = true;
            state.tokenStart = -1;
            state.sel = 0;
        }
        int callback(ImGuiInputTextCallbackData *d)
        {
            VarCompleteState *state = static_cast<VarCompleteState *>(d->UserData);
            // Focus moved to a different InputText -> forget everything.
            if (state->owner != state->pendingId)
            {
                state->owner = state->pendingId;
                state->open = false;
                state->dismissed = false;
                state->sel = 0;
                state->tokenStart = -1;
                state->filter.clear();
                state->prevFilter.clear();
                state->matches.clear();
            }

            switch (d->EventFlag)
            {
            case ImGuiInputTextFlags_CallbackAlways:
            {
                if (state->commit)
                {
                    state->commit = false;
                    apply_in_callback(d, *state);
                    break;
                }
                const int prevStart = state->tokenStart;
                const bool found = scan_token(d->Buf, d->CursorPos, *state);
                if (state->tokenStart != prevStart)
                    state->dismissed = false;

                if (state->filter != state->prevFilter)
                {
                    state->sel = 0;
                    state->prevFilter = state->filter;
                }

                state->open = found && !state->dismissed;
                if (state->open)
                {
                    state->sel = boost::algorithm::clamp(state->sel, 0, static_cast<int>(state->matches.size()) - 1);

                    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
                        apply_in_callback(d, *state);
                    else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
                    {
                        state->dismissed = true;
                        state->open = false;
                    }
                }

                break;
            }
            case ImGuiInputTextFlags_CallbackCompletion:
            {
                if (state->open)
                    apply_in_callback(d, *state);
                break;
            }
            case ImGuiInputTextFlags_CallbackHistory:
            {
                if (state->open && !state->matches.empty())
                {
                    const int n = static_cast<int>(state->matches.size());
                    if (d->EventKey == ImGuiKey_UpArrow)
                        --state->sel;
                    else if (d->EventKey == ImGuiKey_DownArrow)
                        ++state->sel;
                    state->sel = (state->sel % n + n) % n;
                    state->scrollToSel = true;
                }
                break;
            }
            default:
                break;
            }

            return 0;
        }
        const Var *find_var(const std::string &name, const EnvVars &vars)
        {
            const auto it =
                boost::range::find_if(vars, [&name](const Var &v) { return boost::iequals(var_key(v), name); });
            return it == vars.end() ? nullptr : &*it;
        }
    } // namespace internal

} // namespace avUi