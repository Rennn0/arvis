#include <av_ui/json_tree_view.hpp>

#include <cctype>
#include <cfloat>
#include <cmath>
#include <string>
#include <algorithm>

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
        if (key.empty() || (key.front() >= '0' && key.front() <= '9'))
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
        for (std::size_t i = 0; i < n; i++)
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
            this->stats.max_depth = std::max(this->stats.max_depth, static_cast<size_t>(r.depth + 1));
        }
    }

    void JsonTreeView::apply_filter()
    {
        const size_t n = this->rows.size();
        this->matched.assign(n, 0);
        this->keep.assign(n, 0);
        this->hits = 0;
        this->visible_dirty = true;

        if (this->filter.empty())
        {
            std::fill(this->keep.begin(), this->keep.end(), static_cast<uint8_t>(1));
            return;
        }

        // 1. direct hits. Containers are matched on their key only — matching their "{ 5 }"
        //    summary would make every digit in the search box light up half the document
        for (size_t i = 0; i < n; i++)
        {
            const Row &row = this->rows[i];
            const bool hit = boost::algorithm::icontains(row.label, this->filter) ||
                             (!row.container && boost::algorithm::icontains(row.text, this->filter));
            this->matched[i] = static_cast<uint8_t>(hit);
            this->hits += hit ? 1u : 0u;
        }

        // 2. forward pass — everything *under* a hit is kept wholesale. This is the "need
        //    whole node" fix: a node that matches drags its entire subtree along instead of
        //    rendering empty. parent < i, so one sweep in document order is enough.
        for (size_t i = 0; i < n; i++)
        {
            const int32_t parent = this->rows[i].parent;
            const bool under = parent >= 0 && this->keep[static_cast<size_t>(parent)] != 0;
            this->keep[i] = static_cast<uint8_t>(this->matched[i]) || under;
        }

        // 3. reverse pass — ancestors of anything kept stay as breadcrumbs, so the path down
        //    to a hit is still readable. Writing only to parents (which have a lower index)
        //    means a single reverse sweep propagates all the way to the root.

        for (size_t i = n; i-- > 0;)
        {
            if (!this->keep[i])
                continue;
            const int32_t parent = this->rows[i].parent;
            if (parent >= 0)
                this->keep[static_cast<size_t>(parent)] = 1;
        }

        // 4. seed (not force) the fold state so hits are visible without manual clicking.
        //    Because this writes once per filter edit rather than every frame, a node folded
        //    afterwards stays folded and clearing the filter doesn't re-collapse the tree
        for (size_t i = 0; i < n; i++)
        {
            if (this->keep[i])
                this->open[i] = 1;
        }
    }

    void JsonTreeView::rebuild_visible()
    {
        this->visible.clear();
        const size_t n = this->rows.size();
        for (size_t i = 0; i < n;)
        {
            const Row &row = this->rows[i];
            if (!this->keep[i])
            {
                i = row.subtree_end; // no hit anywhere below either — skip the whole branch
                continue;
            }

            this->visible.push_back(i);
            i = (row.container && !this->open[i]) ? row.subtree_end : i + 1;
        }

        this->visible_dirty = false;
        this->mini_rows = 0; // the row list moved; the minimap bands have to be rebuilt
    }

    void JsonTreeView::set_subtree_open(std::int32_t index, bool state)
    {
        const Row &row = this->rows[index];
        for (size_t i = index; i < row.subtree_end; i++)
            this->open[i] = state;
        this->visible_dirty = true;
    }

    std::string JsonTreeView::path_of(std::int32_t index) const
    {
        // walk up to (but not including) the synthetic root row, then unwind.
        boost::container::small_vector<std::int32_t, 16> chain;
        for (std::int32_t i = index; i > 0; i = this->rows[static_cast<std::size_t>(i)].parent)
            chain.push_back(i);

        std::string out;
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            const std::string &label = this->rows[static_cast<std::size_t>(*it)].label;
            if (!label.empty() && label.front() == '[')
            {
                out += label; // array index, already bracketed
            }
            else if (is_identifier(label))
            {
                out.push_back('.');
                out += label;
            }
            else
            {
                std::string quoted = label;
                boost::algorithm::replace_all(quoted, "\\", "\\\\");
                boost::algorithm::replace_all(quoted, "\"", "\\\"");
                out += "[\"" + quoted + "\"]";
            }
        }

        return out.empty() ? std::string(".") : out;
    }

    void JsonTreeView::render_row(std::int32_t index)
    {
        using namespace ImGui;
        const Row &row = this->rows[index];

        TableNextRow();
        TableSetColumnIndex(0);
        PushID(index);

        const float indent = row.depth * indent_width;
        if (index > 0)
            Indent(indent);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (!row.container)
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

        const bool hit = !this->filter.empty() && this->matched[index] != 0;
        if (hit)
            PushStyleColor(ImGuiCol_Text, col_hit);
        SetNextItemOpen(this->open[index] != 0, ImGuiCond_Always);
        const bool nowOpen = TreeNodeEx(row.label.c_str(), flags);
        if (hit)
            PopStyleColor();
        if (row.container)
        {
            const bool isOpen = this->open[index] != 0;
            if (GetIO().KeyAlt && IsItemClicked(ImGuiMouseButton_Left))
            {
                this->set_subtree_open(index, !isOpen);
            }
            else if (nowOpen != isOpen)
            {
                this->open[index] = nowOpen;
                this->visible_dirty = true;
            }
        }
        this->node_context_menu(index);
        if (indent > 0)
            Unindent(indent);

        TableSetColumnIndex(1);
        const float colW = GetContentRegionAvail().x;
        PushStyleColor(ImGuiCol_Text, row.container ? col_meta : kind_color(row.kind));
        TextUnformatted(row.text.c_str(), row.text.c_str() + row.text.size());
        PopStyleColor();
        if (IsItemHovered())
        {
            const ImVec2 size = CalcTextSize(row.text.c_str(), row.text.c_str() + row.text.size());
            if (size.x > colW || row.truncated)
            {
                BeginTooltip();
                PushTextWrapPos(GetFontSize() * 40.f);
                TextUnformatted(row.text.c_str(), row.text.c_str() + row.text.size());
                if (row.truncated)
                    TextDisabled("(truncated for display - right click > copy value for the full text)");
                PopTextWrapPos();
                EndTooltip();
            }
        }
        PopID();
    }

    void JsonTreeView::node_context_menu(std::int32_t index)
    {
        using namespace ImGui;
        if (!BeginPopupContextItem())
            return;
        const Row &row = this->rows[index];
        const nlohmann::json *val = row.value;
        if (MenuItem("copy key"))
            SetClipboardText(row.label.c_str());
        if (MenuItem("copy value"))
            SetClipboardText((val->is_string() ? val->get<std::string>() : val->dump(2)).c_str());
        if (MenuItem("copy key + value"))
            SetClipboardText((row.label + ": " + (val->is_string() ? val->get<std::string>() : val->dump(2))).c_str());
        if (MenuItem("copy path"))
            SetClipboardText(this->path_of(index).c_str());
        if (row.container)
        {
            Separator();
            if (MenuItem("expand subtree"))
                this->set_subtree_open(index, true);
            if (MenuItem("collapse subtree"))
                this->set_subtree_open(index, false);
        }
        EndPopup();
    }

    void JsonTreeView::build_minimap(float stripH)
    {
        const size_t count = this->visible.size();
        const size_t bands = static_cast<size_t>(std::max(1.f, std::floor(stripH / minimap_band_px)));
        const size_t want = std::min(count, bands);

        if (want == this->mini.size() && count == this->mini_rows && stripH == this->mini_height)
            return;

        this->mini.assign(want, MiniBand{});
        this->mini_rows = count;
        this->mini_height = stripH;
        if (want == 0)
            return;

        for (size_t i = 0; i < count; i++)
        {
            const size_t b = std::min(want - 1, i * want / count);
            const size_t index = this->visible[i];
            const Row &row = this->rows[index];
            MiniBand &mb = this->mini[b];
            if (mb.rows++ == 0)
            {
                mb.depth = std::min<int>(row.depth, 255);
                mb.kind = row.kind;
            }
            mb.match = mb.match || this->matched[index] != 0;
        }
    }

    void JsonTreeView::render_minimap(float w, float h)
    {
        using namespace ImGui;
        InvisibleButton("##jsonminimap", ImVec2(w, std::max(h, 1.f)));
        const ImVec2 pMin = GetItemRectMin();
        const ImVec2 pMax = GetItemRectMax();
        const float stripH = pMax.y - pMin.y;
        if (stripH <= 0)
            return;

        this->build_minimap(stripH);
        ImDrawList *draw = GetWindowDrawList();
        draw->AddRectFilled(pMin, pMax, GetColorU32(ImGuiCol_FrameBg), 2.f);
        if (!this->mini.empty())
        {
            const float bandH = stripH / this->mini.size();
            for (size_t b = 0; b < this->mini.size(); b++)
            {
                const MiniBand &band = this->mini[b];
                const float y = pMin.y + b * bandH;
                const float barh = std::max(1.f, bandH - .5f);

                const float indent = std::min<float>(band.depth, 6) * 1.5;
                const ImVec4 c = kind_color(band.kind);
                draw->AddRectFilled(ImVec2(pMin.x + 2.f + indent, y), ImVec2(pMax.x - 2.f, y + barh),
                                    GetColorU32(ImVec4(c.x, c.y, c.z, 0.55f)));

                // filter hits get a full-width tick so they are findable without scrolling.
                if (band.match)
                    draw->AddRectFilled(ImVec2(pMin.x, y), ImVec2(pMax.x, y + std::max(2.f, bandH)),
                                        GetColorU32(col_hit));
            }
        }
        // content_h == scroll_max_y + view_h, so the on-screen slice needs no row maths.
        const float content_h = this->scroll_max_y + this->view_h;
        const float span = content_h > 0.f ? boost::algorithm::clamp(this->view_h / content_h, 0.f, 1.f) : 1.f;
        if (content_h > 0.f)
        {
            const float top = boost::algorithm::clamp(this->scroll_y / content_h, 0.f, 1.f);
            const ImVec2 vmin(pMin.x, pMin.y + top * stripH);
            const ImVec2 vmax(pMax.x, pMin.y + std::min(1.f, top + span) * stripH);
            draw->AddRectFilled(vmin, vmax, GetColorU32(ImGuiCol_TextSelectedBg, 0.45f));
            draw->AddRect(vmin, vmax, GetColorU32(ImGuiCol_Border));
        }

        // click or drag anywhere on the strip to jump there, centred on the cursor. The table
        // is already closed for this frame, so the scroll is queued and applied on the next.
        if (IsItemActive())
        {
            const float local = boost::algorithm::clamp((GetIO().MousePos.y - pMin.y) / stripH, 0.f, 1.f);
            const float usable = std::max(1.f - span, 0.0001f);
            this->pending_scroll = boost::algorithm::clamp((local - span * .5f) / usable, 0.f, 1.f);
        }
        if (IsItemHovered())
            SetMouseCursor(ImGuiMouseCursor_Hand);
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
