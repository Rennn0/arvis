#include <av_ui/json_tree_view.hpp>

#include <cctype>
#include <cfloat>
#include <string>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace
{
    // node colouring by JSON type — keys stay neutral, values are tinted so the
    // structure reads at a glance.
    const ImVec4 col_string = ImVec4(0.55f, 0.80f, 0.55f, 1.f);
    const ImVec4 col_number = ImVec4(0.55f, 0.75f, 0.95f, 1.f);
    const ImVec4 col_bool = ImVec4(0.90f, 0.70f, 0.40f, 1.f);
    const ImVec4 col_null = ImVec4(0.60f, 0.60f, 0.60f, 1.f);
    const ImVec4 col_meta = ImVec4(0.55f, 0.55f, 0.55f, 1.f);

    ImVec4 value_color(const nlohmann::json &v)
    {
        if (v.is_string())
            return col_string;
        if (v.is_boolean())
            return col_bool;
        if (v.is_number())
            return col_number;
        if (v.is_null())
            return col_null;
        return col_meta;
    }

    // scalar as shown in the value column: dump() quotes strings and renders
    // numbers / bool / null verbatim.
    std::string scalar_display(const nlohmann::json &v)
    {
        return v.dump();
    }

    // scalar as matched by the filter: unquoted for strings so "arvis" matches
    // the value "arvis" without the user typing the quotes.
    std::string scalar_raw(const nlohmann::json &v)
    {
        return v.is_string() ? v.get<std::string>() : v.dump();
    }

    bool icontains(std::string_view haystack, std::string_view needle)
    {
        if (needle.empty())
            return true;
        if (needle.size() > haystack.size())
            return false;

        const auto lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i)
        {
            std::size_t j = 0;
            for (; j < needle.size(); ++j)
                if (lower(haystack[i + j]) != lower(needle[j]))
                    break;
            if (j == needle.size())
                return true;
        }
        return false;
    }
} // namespace

namespace avUi
{
    JsonTreeView::JsonTreeView() = default;
    JsonTreeView::~JsonTreeView() = default;

    void JsonTreeView::set_source(std::string_view text)
    {
        this->valid = false;
        this->doc = nlohmann::json();
        this->pretty.clear();
        this->stats = Stats{};

        // parse without exceptions: a non-JSON body is the common case (HTML, plain
        // text, curl error strings) and must not throw on every response.
        nlohmann::json parsed = nlohmann::json::parse(text, nullptr, false);
        if (parsed.is_discarded())
            return;

        this->doc = std::move(parsed);
        this->valid = true;
        this->pretty = this->doc.dump(2);
        this->stats.bytes = text.size();
        this->compute_stats();
    }

    void JsonTreeView::compute_stats()
    {
        const std::size_t bytes = this->stats.bytes;
        this->stats = Stats{};
        this->stats.bytes = bytes;
        this->walk_stats(this->doc, 1);
    }

    void JsonTreeView::walk_stats(const nlohmann::json &value, std::size_t depth)
    {
        this->stats.nodes++;
        if (depth > this->stats.max_depth)
            this->stats.max_depth = depth;

        if (value.is_object())
        {
            this->stats.objects++;
            for (auto it = value.begin(); it != value.end(); ++it)
            {
                this->stats.keys++;
                this->walk_stats(it.value(), depth + 1);
            }
        }
        else if (value.is_array())
        {
            this->stats.arrays++;
            for (const auto &el : value)
                this->walk_stats(el, depth + 1);
        }
        else
        {
            this->stats.leaves++;
        }
    }

    bool JsonTreeView::node_visible(const std::string &label, const nlohmann::json &value) const
    {
        if (this->filter.empty())
            return true;
        if (icontains(label, this->filter))
            return true;

        if (value.is_object() || value.is_array())
        {
            for (auto it = value.begin(); it != value.end(); ++it)
            {
                const std::string child_label = value.is_object() ? it.key() : std::string();
                if (this->node_visible(child_label, it.value()))
                    return true;
            }
            return false;
        }

        return icontains(scalar_raw(value), this->filter);
    }

    void JsonTreeView::node_context_menu(const std::string &label, const nlohmann::json &value) const
    {
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("copy key"))
                ImGui::SetClipboardText(label.c_str());
            if (ImGui::MenuItem("copy value"))
            {
                const std::string s = value.is_string() ? value.get<std::string>() : value.dump(2);
                ImGui::SetClipboardText(s.c_str());
            }
            if (ImGui::MenuItem("copy key + value"))
            {
                const std::string s = label + ": " + (value.is_string() ? value.get<std::string>() : value.dump(2));
                ImGui::SetClipboardText(s.c_str());
            }
            ImGui::EndPopup();
        }
    }

    void JsonTreeView::render_node(const std::string &label, const nlohmann::json &value, int uid)
    {
        if (!this->node_visible(label, value))
            return;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::PushID(uid);

        const bool container = value.is_object() || value.is_array();
        if (container)
        {
            // expand/collapse-all wins for the frame its button was pressed; otherwise an
            // active filter forces matches open so hits are visible without manual clicking.
            if (this->set_open_all != -1)
                ImGui::SetNextItemOpen(this->set_open_all == 1);
            else if (!this->filter.empty())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);

            const bool open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_SpanAllColumns);
            this->node_context_menu(label, value);

            ImGui::TableSetColumnIndex(1);
            // ImGui::TextColored(col_meta, value.is_object() ? "{ %zu }" : "[ %zu ]", value.size());

            if (open)
            {
                int child_uid = 0;
                if (value.is_object())
                {
                    for (auto it = value.begin(); it != value.end(); ++it)
                        this->render_node(it.key(), it.value(), child_uid++);
                }
                else
                {
                    for (std::size_t i = 0; i < value.size(); ++i)
                        this->render_node("[" + std::to_string(i) + "]", value[i], child_uid++);
                }
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet |
                                                 ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                 ImGuiTreeNodeFlags_SpanAllColumns);
            this->node_context_menu(label, value);

            ImGui::TableSetColumnIndex(1);
            const std::string text = scalar_display(value);
            // wrap long scalar values (URLs, tokens, prose) at the value cell's right edge
            // instead of overflowing; the table row grows to fit the wrapped lines.
            ImGui::PushStyleColor(ImGuiCol_Text, value_color(value));
            ImGui::PushTextWrapPos(0.0f); // 0 == wrap at the current cell's right edge
            ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
    }

    void JsonTreeView::render_tree()
    {
        ImGui::SetNextItemWidth(240.f);
        ImGui::InputTextWithHint("##json_search", "search keys / values", &this->filter);
        ImGui::SameLine();
        if (ImGui::SmallButton("expand all"))
            this->set_open_all = 1;
        ImGui::SameLine();
        if (ImGui::SmallButton("collapse all"))
            this->set_open_all = 0;
        ImGui::SameLine();
        if (ImGui::SmallButton("copy json"))
            ImGui::SetClipboardText(this->pretty.c_str());

        const float stats_h = ImGui::GetFrameHeightWithSpacing();
        const ImGuiTableFlags flags = /*ImGuiTableFlags_RowBg |*/ ImGuiTableFlags_BordersInnerV |
                                      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("##json_tree", 2, flags, ImVec2(0, -stats_h)))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            this->render_node("root", this->doc, 0);
            ImGui::EndTable();
        }

        // the expand/collapse-all pulse only applies for the frame it was pressed.
        this->set_open_all = -1;

        ImGui::TextDisabled("%zu nodes  |  depth %zu  |  %zu obj  |  %zu arr  |  %zu keys  |  %zu leaves  |  %zu B",
                            this->stats.nodes, this->stats.max_depth, this->stats.objects, this->stats.arrays,
                            this->stats.keys, this->stats.leaves, this->stats.bytes);
    }

    void JsonTreeView::render_pretty()
    {
        if (ImGui::SmallButton("copy all"))
            ImGui::SetClipboardText(this->pretty.c_str());

        // read-only multiline input so the user can still select and copy arbitrary
        // ranges of the pretty-printed text.
        ImGui::InputTextMultiline("##json_pretty", &this->pretty, ImGui::GetContentRegionAvail(),
                                  ImGuiInputTextFlags_ReadOnly);
    }
} // namespace avUi
