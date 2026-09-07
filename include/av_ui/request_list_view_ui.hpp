#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_request.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_root/av_request_list_state.hpp>
#include <av_ui/tab_bar_ui.hpp>
#include <av_s/av_request_storage.hpp>

namespace avUi
{
    class RequstListViewUi : public avR::UiComponent
    {
    public:
        explicit RequstListViewUi(std::string id);
        explicit RequstListViewUi(std::string id, avR::AvState *sharedState);
        ~RequstListViewUi();

    private:
        void render() override;
        void update() override;

        ImGuiWindowFlags windowFlags;
        avR::AvInterViewSharedState *shared_state;
        std::string filter_text;
        std::shared_ptr<avR::AvRequestListState> request_list_state;
        std::unique_ptr<avS::AvRequestStorage> request_storage;
        std::unique_ptr<avUi::TabBarUi> tabs;

        bool _tab_badge_enabled;
        int _tab_history_counter_badge = 0;
        int _tab_saved_counter_badge = 0;

        std::optional<int64_t> pending_delete_req;

        /// @brief A history-tree mutation, applied after the row loop. Deferred for the same
        ///        reason as pending_delete_req: the vector being iterated must not be
        ///        modified mid-frame, and the popup that requests it is drawn inside the loop.
        struct HistoryAction
        {
            enum class Kind
            {
                none,
                remove, ///< drop one snapshot
                clear   ///< drop every snapshot of one saved request
            };

            Kind kind = Kind::none;
            int64_t origin_id = 0;
            int64_t snapshot_id = 0;
        };
        HistoryAction pending_history_action;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);
        void render_tab_history(const ImGuiStyle &style);
        void render_tab_saved(const ImGuiStyle &style);

        /// @brief Row for a saved request. Only ever called with an entry owned by
        ///        request_list_state->requests - it renames/deletes by DB id, which is
        ///        meaningless for a snapshot. Snapshots use render_history_row.
        void render_request_row(const avR::AvRequest *selected, avR::AvRequest *request, const ImGuiStyle &style);

        // --- history tree ---------------------------------------------------------
        void render_history_group(avR::AvRequest *origin, const ImGuiStyle &style);
        void render_history_row(const avR::AvRequest *selected, const std::shared_ptr<avR::AvRequest> &snap,
                                const ImGuiStyle &style, float rail_x);
        /// @brief Select a snapshot, taking a strong reference so it outlives trimming.
        void select_snapshot(const std::shared_ptr<avR::AvRequest> &snap);
        /// @brief Select a saved request, releasing any snapshot reference held.
        void select_request(avR::AvRequest *request);
        void apply_history_action();
        void apply_pending_delete();

        void new_request();
    };
} // namespace avUi
