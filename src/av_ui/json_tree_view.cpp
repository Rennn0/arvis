#include <av_ui/json_tree_view.hpp>

#include <cctype>
#include <cfloat>
#include <string>

#include <boost/algorithm/clamp.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/small_vector.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace
{
    // node colouring by JSON type — keys stay neutral, values are tinted so the
    // structure reads at a glance.
    constexpr ImVec4 col_string = Rgb(0x8ccc8c);
    constexpr ImVec4 col_number = Rgb(0x8cbff2);
    constexpr ImVec4 col_bool = Rgb(0xe6b366);
    constexpr ImVec4 col_null = Rgb(0x999999);
    constexpr ImVec4 col_meta = Rgb(0x8c8c8c);
    constexpr ImVec4 col_hit = Rgb(0xf2c759);

    // one indent step per depth level in the key column.
    const float indent_width = 12.f;
    // the preview strip on the right edge, and how many pixels one of its bands covers.
    const float minimap_width = 14.f;
    const float minimap_band_px = 2.f;
    // scalar text longer than this is cached truncated: keeps per-row memory bounded and
    // CalcTextSize cheap on hover. The untruncated value is still one "copy value" away.
    const std::size_t max_text_len = 4096;
    // documents at or below this many nodes arrive expanded one level deep; bigger ones open
    // only the root, so a 50k-node response doesn't unfold into a wall of rows.
    const std::size_t auto_expand_max_rows = 500;

    const ImVec4 &kind_color(avUi::JsonKind kind)
    {
        switch (kind)
        {
        case avUi::JsonKind::string:
            return col_string;
        case avUi::JsonKind::number:
            return col_number;
        case avUi::JsonKind::boolean:
            return col_bool;
        case avUi::JsonKind::null:
            return col_null;
        default:
            return col_meta;
        }
    }

    avUi::JsonKind kind_of(const nlohmann::json *value)
    {
        if (value->is_object())
            return avUi::JsonKind::object;
        if (value->is_array())
            return avUi::JsonKind::array;
        if (value->is_string())
            return avUi::JsonKind::string;
        if (value->is_boolean())
            return avUi::JsonKind::boolean;
        if (value->is_number())
            return avUi::JsonKind::number;
        return avUi::JsonKind::null;
    }

    // a key that can be written as `.name` in a path instead of `["name"]`.
    bool is_identifier(const std::string &key)
    {
        if (key.empty() || (key.front() >= '0' && key.back() <= '9'))
            return false;

        return boost::algorithm::all_of(key, boost::is_alnum() || boost::is_any_of("_$"));
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
        this->rows.clear();
        this->open.clear();
        this->matched.clear();
        this->keep.clear();
        this->visible.clear();
        this->mini.clear();
        this->mini_rows = 0;
        this->hits = 0;
        this->visible_dirty = true;
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
        this->build_rows();
        this->compute_stats();
        this->apply_filter();
    }

    void JsonTreeView::build_rows()
    {
        this->rows.reserve(this->stats.bytes / 16 + 8);
        this->flatten(&this->doc, "root", -1, 0);
        const std::size_t n = this->rows.size();
        this->open.assign(n, 0);
        this->matched.assign(n, 0);
        this->keep.assign(n, 1);
        const std::uint16_t seed_dep = (n <= auto_expand_max_rows) ? 1 : 0;
        for (std::size_t i = 0; i < 0; i++)
            this->open[i] = static_cast<std::uint8_t>(this->rows[i].depth <= seed_dep);
        this->visible_dirty = true;
    }

    void JsonTreeView::flatten(const nlohmann::json *value, std::string label, std::int32_t parent, std::uint16_t depth)
    {
        const size_t self = this->rows.size();
        this->rows.push_back(Row{});
        {
            Row &row = this->rows[self];
            row.value = value;
            row.label = std::move(label);
            row.parent = parent;
            row.depth = depth;
            row.kind = kind_of(value);
            row.container = value->is_object() || value->is_array();
            if (row.container)
                row.text = (value->is_object() ? "{ " : "[ ") + std::to_string(value->size()) +
                           (value->is_object() ? " }" : " ]");
            else
            {
                row.text = value->dump();
                if (row.text.size() > max_text_len)
                {
                    row.text.resize(max_text_len);
                    row.text += "...";
                    row.truncated = true;
                }
            }
        }

        if (value->is_object())
            for (auto it = value->begin(); it != value->end(); it++)
                this->flatten(&it.value(), it.key(), self, depth + 1);
        else if (value->is_array())
        {
            size_t index = 0;
            for (const auto &el : *value)
                this->flatten(&el, "[" + std::to_string(index++) + "]", self, depth + 1);
        }
        this->rows[self].subtree_end = static_cast<int32_t>(this->rows.size());
    }

    void JsonTreeView::compute_stats()
    {
        const std::size_t bytes = this->stats.bytes;
        this->stats = Stats{};
        this->stats.bytes = bytes;
        this->stats.nodes = this->rows.size();
        for (const Row &r : this->rows)
        {
            if (r.kind == JsonKind::object)
                this->stats.objects++;
            else if (r.kind == JsonKind::array)
                this->stats.arrays++;
            else
                this->stats.leaves++;
            if (r.parent >= 0 && this->rows[r.parent].kind == JsonKind::object)
                this->stats.keys++;
            this->stats.max_depth == std::max(this->stats.max_depth, static_cast<size_t>(r.depth + 1));
        }
    }

    void JsonTreeView::apply_filter()
    {
    }

    void JsonTreeView::rebuild_visible()
    {
    }

    void JsonTreeView::set_subtree_open(std::int32_t index, bool state)
    {
    }

    std::string JsonTreeView::path_of(std::int32_t index) const
    {
        return std::string();
    }

    void JsonTreeView::render_row(std::int32_t index)
    {
    }

    void JsonTreeView::node_context_menu(std::int32_t index)
    {
    }

    void JsonTreeView::build_minimap(float stripH)
    {
    }

    void JsonTreeView::render_minimap(float w, float h)
    {
    }

    void JsonTreeView::render_pretty()
    {
        if (ImGui::Button("copy all"))
            ImGui::SetClipboardText(this->pretty.c_str());

        // read-only multiline input so the user can still select and copy arbitrary
        // ranges of the pretty-printed text.
        ImGui::InputTextMultiline("##json_pretty", &this->pretty, ImGui::GetContentRegionAvail(),
                                  ImGuiInputTextFlags_ReadOnly);
    }
    void JsonTreeView::render_tree()
    {
        if (!this->valid || this->rows.empty())
        {
            ImGui::TextDisabled("no json to show");
            return;
        }

        // ---- toolbar -----------------------------------------------------------------
        ImGui::SetNextItemWidth(240.f);
        if (ImGui::InputTextWithHint("##json_search", "search keys / values", &this->filter))
            this->apply_filter();

        ImGui::SameLine();
        if (ImGui::ArrowButton("##expand_all", ImGuiDir_Down))
        {
            std::fill(this->open.begin(), this->open.end(), static_cast<std::uint8_t>(1));
            this->visible_dirty = true;
        }
        ImGui::SetItemTooltip("expand all");

        ImGui::SameLine();
        if (ImGui::ArrowButton("##collapse_all", ImGuiDir_Up))
        {
            std::fill(this->open.begin(), this->open.end(), static_cast<std::uint8_t>(0));
            this->visible_dirty = true;
        }
        ImGui::SetItemTooltip("collapse all (alt+click a node folds just that branch)");

        ImGui::SameLine();
        if (ImGui::Button("copy json"))
            ImGui::SetClipboardText(this->pretty.c_str());

        ImGui::SameLine();
        ImGui::Checkbox("map", &this->show_minimap);
        ImGui::SetItemTooltip("document preview on the right edge");

        if (!this->filter.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(this->hits > 0 ? col_hit : col_meta, "%zu hits", this->hits);
        }

        // ---- tree --------------------------------------------------------------------
        if (this->visible_dirty)
            this->rebuild_visible();

        const ImGuiStyle &style = ImGui::GetStyle();
        const float stats_h = ImGui::GetFrameHeightWithSpacing();
        const float strip_w = this->show_minimap ? minimap_width + style.ItemSpacing.x : 0.f;
        const float table_w = ImGui::GetContentRegionAvail().x - strip_w;

        const ImGuiTableFlags flags =
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("##json_tree", 2, flags, ImVec2(table_w, -stats_h)))
        {
            ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.55f);

            // with ScrollY the table owns a child window and it is the current window here,
            // so these scroll calls act on the table's own scrolling region.
            if (this->pending_scroll >= 0.f)
            {
                ImGui::SetScrollY(this->pending_scroll * ImGui::GetScrollMaxY());
                this->pending_scroll = -1.f;
            }

            // rows are uniform height, so the clipper submits ~60 of them however big the
            // document is. Let it measure the height itself (tables add cell padding).
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(this->visible.size()));
            while (clipper.Step())
                for (int k = clipper.DisplayStart; k < clipper.DisplayEnd; ++k)
                    this->render_row(this->visible[static_cast<std::size_t>(k)]);

            this->scroll_y = ImGui::GetScrollY();
            this->scroll_max_y = ImGui::GetScrollMaxY();
            this->view_h = ImGui::GetWindowHeight();

            ImGui::EndTable();
        }

        if (this->show_minimap)
        {
            const float table_h = ImGui::GetItemRectSize().y;
            ImGui::SameLine(0.f, style.ItemSpacing.x);
            this->render_minimap(minimap_width, table_h);
        }

        ImGui::TextDisabled("%zu nodes  |  depth %zu  |  %zu obj  |  %zu arr  |  %zu keys  |  %zu leaves  |  %zu B",
                            this->stats.nodes, this->stats.max_depth, this->stats.objects, this->stats.arrays,
                            this->stats.keys, this->stats.leaves, this->stats.bytes);
    }
} // namespace avUi
