#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <nlohmann/json.hpp>

#include "helpers_ui.h"

namespace avUi
{
    enum class JsonKind : std::uint8_t
    {
        object,
        array,
        string,
        number,
        boolean,
        null
    };
    /// @brief Read-only JSON viewer widget: a collapsible tree with per-node copy,
    ///        a text filter over keys/values, expand/collapse-all, a pretty-printed
    ///        view, and summary statistics.
    ///
    /// Parse once via set_source() (call it only when the underlying text actually
    /// changes), then call render_tree() / render_pretty() each frame. This is a
    /// plain widget the detailed request view composes into its footer — deliberately
    /// NOT a UiComponent, since it is data-driven per response rather than a node in
    /// the retained tree.
    class JsonTreeView
    {
    public:
        JsonTreeView();
        ~JsonTreeView();

        /// @brief (re)parse @p text as JSON and cache the document, its pretty dump
        ///        and its statistics. Non-JSON input leaves is_json() == false.
        void set_source(std::string_view text);

        /// @brief did the last set_source() parse as valid JSON?
        bool is_json() const noexcept { return this->valid; }

        /// @brief toolbar (search / expand / collapse / copy) + collapsible tree +
        ///        stats line. Assumes is_json().
        void render_tree();

        /// @brief pretty-printed (indent = 2) body in a read-only, selectable text
        ///        box with a copy-all button. Assumes is_json().
        void render_pretty();

    private:
        struct Row
        {
            const nlohmann::json *value = nullptr;
            std::string label;
            std::string text;
            std::int32_t parent = -1;
            std::int32_t subtree_end = 0;
            std::uint16_t depth = 0;
            JsonKind kind = JsonKind::null;
            bool container = false;
            bool truncated = false;
        };

        struct MiniBand
        {
            std::uint32_t rows = 0;
            std::uint8_t depth = 0;
            JsonKind kind = JsonKind::null;
            bool match = false;
        };

        struct Stats
        {
            std::size_t nodes = 0;
            std::size_t objects = 0;
            std::size_t arrays = 0;
            std::size_t keys = 0;
            std::size_t leaves = 0;
            std::size_t max_depth = 0;
            std::size_t bytes = 0;
        } stats;

        nlohmann::json doc;
        bool valid = false;
        std::string pretty; ///< cached dump(2), reused by the pretty view and copy-all
        std::string filter; ///< current search text (case-insensitive)

        std::vector<Row> rows;
        std::vector<std::uint8_t> open;    ///< index-parallel to `rows`: is this container unfolded?
        std::vector<std::uint8_t> matched; ///< index-parallel: this row's own key/value matched the filter
        std::vector<std::uint8_t> keep;    ///< index-parallel: a hit, inside a hit, or an ancestor of one
        std::vector<std::int32_t> visible; ///< indices into `rows`, in draw order
        std::size_t hits = 0;              ///< number of direct matches, shown on the stats line
        bool visible_dirty = true;

        std::vector<MiniBand> mini;
        std::size_t mini_rows = 0;
        float mini_height = 0.f;
        bool show_minimap = true;

        // captured inside BeginTable/EndTable (where the current window is the table's own
        // scrolling child) so the minimap, drawn after EndTable, can place its viewport box.
        float scroll_y = 0.f;
        float scroll_max_y = 0.f;
        float view_h = 0.f;
        float pending_scroll = -1.f; ///< >= 0: scroll ratio queued by the minimap, applied next frame

        void build_rows();
        /// @brief
        /// @param value
        /// @param label
        /// @param parent
        /// @param depth
        void flatten(const nlohmann::json *value, std::string label, std::int32_t parent, std::uint16_t depth);
        void compute_stats();
        /// @brief recompute matched/keep/hits for the current filter and seed the open flags
        ///        so hits are reachable without manual clicking.
        void apply_filter();
        /// @brief collapse `rows` down to the indices that should be drawn, honouring both
        ///        the filter (keep) and the fold state (open).
        void rebuild_visible();
        /// @brief fold or unfold @p index and every descendant in one range write.
        void set_subtree_open(std::int32_t index, bool state);
        /// @brief jq / JS style path to a node, e.g. `.data.items[0].name`; "." for the root.
        std::string path_of(std::int32_t index) const;
        void render_row(std::int32_t index);
        void node_context_menu(std::int32_t index);
        void build_minimap(float stripH);
        void render_minimap(float w, float h);
    };
} // namespace avUi
