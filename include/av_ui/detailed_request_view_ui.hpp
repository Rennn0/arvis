#pragma once
#include <string_view>
#include <future>
#include <av_root/av_request.hpp>
#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_s/av_request_storage.hpp>
#include <av_s/av_request_params_storage.hpp>
#include <av_s/av_request_headers_storage.hpp>
#include <av_s/av_request_cookies_storage.hpp>
#include <av_ui/tab_bar_ui.hpp>

namespace avUi
{
    class JsonTreeView;

    class DetailedRequestViewUi : public avR::UiComponent
    {
    public:
        explicit DetailedRequestViewUi(std::string id);
        explicit DetailedRequestViewUi(std::string id, avR::AvState *sharedState);
        ~DetailedRequestViewUi();

    private:
        float footer_height;

        void render() override;
        void update() override;

        ImGuiWindowFlags window_flags;
        avR::AvInterViewSharedState *shared_state;
        std::unique_ptr<avS::AvRequestStorage> request_storage;
        std::unique_ptr<avS::AvRequestParamsStorage> request_params_storage;
        std::unique_ptr<avS::AvRequestHeadersStorage> request_headers_storage;
        std::unique_ptr<avS::AvRequestCookiesStorage> request_cookies_storage;
        std::unique_ptr<avUi::TabBarUi> _tabs;

        // networking: the request runs off the UI thread so a slow/dead endpoint never
        // freezes the window. pending_response is declared last so it is destroyed first,
        // joining the worker before network_manager / response_body are torn down.
        avNet::NetworkManager network_manager;

        // std::string response_body;
        // long response_http_code;
        // avNet::response_status last_status;
        // bool has_response;

        // snapshot of the request that produced the current response, kept so the footer can
        // reproduce it (e.g. "copy as cURL"). It is an independent copy of what the worker got.

        // how the response body is presented in the footer. Defaults to the JSON tree when the
        // body parses as JSON (chosen in poll_response), otherwise raw text.
        enum class ResponseView
        {
            tree,
            pretty,
            raw,
            res_headers,
            res_cookies
        };
        ResponseView response_view = ResponseView::raw;
        // parses the response body once per response and renders it as a collapsible tree /
        // pretty dump. Held by pointer so its (heavy) JSON header stays out of this header.
        std::unique_ptr<JsonTreeView> json_view;

        // history: the snapshot captured for the send currently in flight. It is pushed
        // into the origin's recent_reqs only once the response lands (poll_response), so a
        // history entry always carries its own status/body instead of an empty one. The
        // origin is remembered by id, not by pointer, so deleting it mid-flight is safe.
        std::shared_ptr<avR::AvRequest> pending_snapshot;
        int64_t pending_snapshot_origin_id = 0;
        // set instead of pending_snapshot when an edited history entry is being re-sent: the
        // result updates that entry in place rather than appending another. Strong reference,
        // so trimming or clearing history while the fetch is in flight cannot free it.
        std::shared_ptr<avR::AvRequest> pending_replay_target;

        // std::future<avNet::response_status> pending_response;
        std::future<avNet::http_result> pending_response_v2;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);

        // fire the current request on a worker thread; poll the future each frame.
        void send_request();
        void poll_response();

        void save_state_change() const;
        void save_changes();

        /// @brief the saved request with this id, or nullptr. Used to route a landed
        ///        response back to the request that was actually sent.
        avR::AvRequest *find_saved_request(int64_t id) const;
        /// @brief drop the oldest snapshots beyond shared_state->recent_req_limit
        void trim_history(avR::AvRequest *origin) const;

        static std::string build_url(std::string_view base_url, const std::vector<avR::AvRequestParam> &params);

        void render_tab_params() const;
        void render_tab_headers() const;
        void render_tab_body() const;
        void render_tab_cookies() const;

        void render_shortcuts() const;
        void render_menu() const;
    };
} // namespace avUi
