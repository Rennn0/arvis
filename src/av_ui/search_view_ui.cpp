#include <av_ui/search_view_ui.hpp>
#include <av_ui/icons_font_awesome7.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cfloat>
#include <cctype>
#include <string_view>
namespace avUi
{
    ImVec2 center;
    constexpr float kListHeight = 340.f;
    constexpr float kMethodColWidth = 56.f;
    // case-insensitive subsequence match with light scoring: consecutive runs, word
    // boundaries and early matches score higher, long haystacks are penalised.
    bool fuzzy_match(std::string_view haystack, std::string_view needle, int &score)
    {
        score = 0;
        if (needle.empty())
            return true;
        if (haystack.empty())
            return false;

        std::size_t h = 0;
        std::size_t n = 0;
        int consecutive = 0;
        int firstHit = -1;
        bool prevBoundary = true;

        // ბიოგრაფია (9/4) ~ 2
        // იფ -> 25 -> 50 -> 49,47 -> 47 score
        // ბ -> 25 -> 23 score
        // გრა -> 25 -> 56 -> 93 -> 90,88 -> 88 score
        while (h < haystack.size() && n < needle.size())
        {
            const unsigned char hc = static_cast<unsigned char>(haystack[h]);
            const unsigned char nc = static_cast<unsigned char>(needle[n]);

            if (std::tolower(hc) == std::tolower(nc))
            {
                if (firstHit < 0)
                    firstHit = static_cast<int>(h);
                score += 10;
                score += consecutive * 6;
                if (prevBoundary)
                    score += 15;
                ++consecutive;
                ++n;
            }
            else
            {
                consecutive = 0;
            }

            prevBoundary = (std::isalnum(hc) == 0);
            ++h;
        }

        if (n != needle.size())
            return false;

        score -= firstHit;
        score -= static_cast<int>(haystack.size()) / 4;
        return true;
    }

    SearchViewUi::~SearchViewUi()
    {
    }

    SearchViewUi::SearchViewUi(std::string id, avR::AvState *sharedState)
        : UiComponent(id), shared_state(static_cast<avR::AvInterViewSharedState *>(sharedState))
    {
        this->shared_state->on_show_search.emplace(
            [this]() { this->shared_state->show_search_view = !this->shared_state->show_search_view; });

        // ---- configurable search fields -------------------------------------------------
        // add/remove entries here; the prefix parser and the hint line pick them up automatically.
        this->fields = {
            {'t', "title", [](const avR::AvRequest &r) { return r.display_name(); }},
            {'m', "method", [](const avR::AvRequest &r) { return avNet::NetworkManager::method_text(r.method); }},
            {'u', "url", [](const avR::AvRequest &r) { return r.url; }},
            {'c', "collection", [](const avR::AvRequest &r) { return r.collection.value_or(""); }},
            {'s', "status",
             [](const avR::AvRequest &r) { return r.status_code ? std::to_string(r.status_code.value()) : ""; }},

            {'b', "body", [](const avR::AvRequest &r) { return r.body.value_or(""); }},
        };

        this->on_pick.emplace([this](const avR::AvRequest *r)
                              { this->shared_state->display_request = const_cast<avR::AvRequest *>(r); });
    }

    void SearchViewUi::render()
    {
        using namespace ImGui;
        if (!this->shared_state->show_search_view)
        {
            this->was_open = false;
            return;
        }

        if (!was_open)
        {
            this->was_open = true;
            this->open_reset();
            ImGui::SetNextWindowFocus();
        }

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs |
                                       ImGuiWindowFlags_AlwaysAutoResize;
        center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("search", &this->shared_state->show_search_view, flags))
        {
            if (this->want_focus)
            {
                ImGui::SetKeyboardFocusHere();
                this->want_focus = false;
            }

            ImGui::SetNextItemWidth(-FLT_MIN);
            const bool committed =
                ImGui::InputTextWithHint("##query", ICON_FA_MAGNIFYING_GLASS "  search requests...", this->query,
                                         sizeof(query), ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::IsItemEdited())
            {
                this->dirty = true;
                this->selected = 0;
                this->want_scroll = true;
            }

            if (this->dirty)
                this->rebuild();

            // ---- keyboard --------------------------------------------------------------
            // single-line InputText does not claim up/down, so plain IsKeyPressed works here.
            if (IsKeyPressed(ImGuiKey_DownArrow, true))
                this->move_selection(1);
            if (IsKeyPressed(ImGuiKey_UpArrow, true))
                this->move_selection(-1);
            if (IsKeyPressed(ImGuiKey_PageDown, true))
                this->move_selection(10);
            if (IsKeyPressed(ImGuiKey_PageUp, true))
                this->move_selection(-10);
            if (IsKeyPressed(ImGuiKey_Escape, false))
            {
                this->close();
                End();
                return;
            }
            if (committed)
            {
                this->accept();
                End();
                return;
            }
            // ---- results ---------------------------------------------------------------
            BeginChild("##results", ImVec2(0.f, kListHeight));
            if (this->hits.empty())
                TextDisabled(ICON_FA_BAN "  no matches");

            for (size_t i = 0; i < this->hits.size(); i++)
            {
                const Hit &hit = this->hits[i];
                PushID(i);

                const ImVec2 row = GetCursorScreenPos();
                if (Selectable("##row", i == this->selected))
                {
                    this->selected = i;
                    this->accept();
                    PopID();
                    EndChild();
                    End();
                    return;
                }

                if (i == this->selected && this->want_scroll)
                {
                    SetScrollHereY(.5f);
                    this->want_scroll = false;
                }

                ImDrawList *dl = GetWindowDrawList();
                float x = row.x + 4.f;
                dl->AddText(ImVec2(x, row.y), ColorConvertFloat4ToU32(this->get_method_color(hit.method)),
                            avNet::NetworkManager::method_text(hit.method));
                x += kMethodColWidth;
                const char *label = hit.label.c_str();
                dl->AddText(ImVec2(x, row.y), GetColorU32(ImGuiCol_Text), label);
                x += CalcTextSize(label).x + 12.f;
                if (!hit.sub.empty())
                    dl->AddText(ImVec2(x, row.y), GetColorU32(ImGuiCol_TextDisabled), hit.sub.c_str());
                PopID();
            }
            EndChild();

            // keep the caret in the box even after a mouse click in the list
            if (!IsAnyItemActive() && !IsAnyItemHovered() && !IsAnyMouseDown)
                this->want_focus = true;

            // ---- hint ------------------------------------------------------------------
            Separator();
            std::string hint;
            for (const auto &f : fields)
                hint += " :" + std::string(1, f.key) + " " + f.name;
            TextDisabled("%d result(s)%s", hits.size(), hint.c_str());

            const bool isFocused = IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
            if (this->open_frames > 0 && !isFocused)
            {
                this->close();
                End();
                return;
            }
            ++this->open_frames;
        }
        ImGui::End();
    }

    const SearchViewUi::SearchField *SearchViewUi::field_for(const std::string &key) const
    {
        if (key.empty())
            return nullptr;

        if (key.size() == 1)
        {
            for (const auto &f : this->fields)
                if (std::tolower(static_cast<char>(key[0])) == f.key)
                    return &f;
        }
        else
        {
            for (const auto &f : this->fields)
                if (boost::istarts_with(f.name, key))
                    return &f;
        }

        return nullptr;
    }
    std::vector<SearchViewUi::QueryTerm> SearchViewUi::parse_query(const std::string &raw) const
    {
        std::vector<QueryTerm> out;
        const SearchField *currentField = &fields[0]; // title
        std::vector<std::string> tokens;
        boost::split(tokens, raw, boost::is_any_of(" \t"), boost::token_compress_on);

        for (std::string &token : tokens)
        {
            boost::trim(token);
            if (token.empty())
                continue;

            if (token[0] == ':')
            {
                std::string key = token.substr(1);
                std::string glued;

                const size_t sep = key.find_first_of("=:");
                if (sep != std::string::npos)
                {
                    glued = key.substr(sep + 1);
                    key = key.substr(0, sep);
                }

                if (const SearchField *f = this->field_for(key))
                {
                    currentField = f;
                    if (!glued.empty())
                        out.emplace_back(QueryTerm{currentField, glued});

                    continue;
                }
            }

            out.emplace_back(QueryTerm{currentField, token});
        }

        return out;
    }
    void SearchViewUi::rebuild()
    {
        this->hits.clear();
        const auto terms = this->parse_query(this->query);
        for (const std::shared_ptr<avR::AvRequest> &r : this->shared_state->request_list_state->requests)
        {
            int total = 0;
            bool matched = true;

            for (const auto &term : terms)
            {
                int s = 0;
                if (!fuzzy_match(term.field->extract(*r), term.text, s))
                {
                    matched = false;
                    break;
                }

                total += s;
            }

            if (matched)
                this->hits.emplace_back(Hit{r->id, total, r->display_name(), r->url, r->method});
        }

        std::stable_sort(this->hits.begin(), this->hits.end(),
                         [](const Hit &a, const Hit &b) { return a.score > b.score; });

        this->selected = std::max(std::min(this->selected, static_cast<int>(hits.size() - 1)), 0);
        dirty = false;
    }
    void SearchViewUi::move_selection(int delta)
    {
        if (this->hits.empty())
            return;

        const int n = this->hits.size();
        this->selected = ((this->selected + delta) % n + n) % n; // 10  --d(1)-> 1, 5 --d(-1)-> 4, 0 --d(-1)-> 4
        this->want_scroll = true;
    }
    void SearchViewUi::accept()
    {
        if (this->selected < 0 || this->selected >= this->hits.size())
            return;

        const int64_t id = this->hits[this->selected].id;
        for (const std::shared_ptr<avR::AvRequest> &r : this->shared_state->request_list_state->requests)
        {
            if (r->id != id)
                continue;
            if (this->on_pick.has_value())
                this->on_pick.value()(r.get());
            break;
        }

        this->close();
    }
    void SearchViewUi::open_reset()
    {
        this->query[0] = '\0';
        this->selected = 0;
        this->dirty = true;
        this->want_focus = true;
        this->want_scroll = false;
        this->open_frames = 0;
    }
    void SearchViewUi::close()
    {
        this->shared_state->show_search_view = false;
        this->was_open = false;
    }
} // namespace avUi
