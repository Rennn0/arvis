#pragma once
#include <string_view>
#include <future>
#include <av_root/av_request.hpp>
#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>
#include <av_s/av_request_storage.hpp>
#include <av_s/av_request_params_storage.hpp>
#include <av_s/av_request_headers_storage.hpp>

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

        ImGuiWindowFlags window_flags;
        avR::AvInterViewSharedState *shared_state;
        std::unique_ptr<avS::AvRequestStorage> request_storage;
        std::unique_ptr<avS::AvRequestParamsStorage> request_params_storage;
        std::unique_ptr<avS::AvRequestHeadersStorage> request_headers_storage;

        // networking: the request runs off the UI thread so a slow/dead endpoint never
        // freezes the window. pending_response is declared last so it is destroyed first,
        // joining the worker before network_manager / response_body are torn down.
        avNet::NetworkManager network_manager;
        std::string response_body;
        long response_http_code;
        avNet::response_status last_status;
        bool has_response;
        // snapshot of the request that produced the current response, kept so the footer can
        // reproduce it (e.g. "copy as cURL"). It is an independent copy of what the worker got.
        avNet::http_request last_request;

        // how the response body is presented in the footer. Defaults to the JSON tree when the
        // body parses as JSON (chosen in poll_response), otherwise raw text.
        enum class ResponseView
        {
            tree,
            pretty,
            raw
        };
        ResponseView response_view = ResponseView::raw;
        // parses the response body once per response and renders it as a collapsible tree /
        // pretty dump. Held by pointer so its (heavy) JSON header stays out of this header.
        std::unique_ptr<JsonTreeView> json_view;

        std::future<avNet::response_status> pending_response;

        void render_header(const ImGuiStyle &style);
        void render_main_content(const ImGuiStyle &style);
        void render_footer(const ImGuiStyle &style);

        // fire the current request on a worker thread; poll the future each frame.
        void send_request();
        void poll_response();

        void save_state_change() const;

        static std::string build_url(std::string_view base_url, const std::vector<avR::AvRequestParam> &params);

        void render_tab_params() const;
        void render_tab_headers() const;
        void render_tab_body() const;
        void render_tab_auth() const;
    };
} // namespace avUi
