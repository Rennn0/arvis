#pragma once
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace avUi
{
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
        nlohmann::json doc;
        bool valid = false;
        std::string pretty;    ///< cached dump(2), reused by the pretty view and copy-all
        std::string filter;    ///< current search text (case-insensitive)
        int set_open_all = -1; ///< -1 none, 0 collapse, 1 expand — applied for a single frame

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

        void compute_stats();
        void walk_stats(const nlohmann::json &value, std::size_t depth);

        void render_node(const std::string &label, const nlohmann::json &value, int uid);
        void node_context_menu(const std::string &label, const nlohmann::json &value) const;

        /// @brief with a filter active, a node is visible when its label matches, its
        ///        scalar value matches, or any descendant matches (so parents of a hit
        ///        stay in the tree). Always visible when the filter is empty.
        bool node_visible(const std::string &label, const nlohmann::json &value) const;
    };
} // namespace avUi
