#pragma once

#include <memory>
#include <optional>
#include <string>
#include <av_net/network_manager.hpp>
#include <vector>

namespace avR
{
    struct AvRequestParam
    {
        bool included = true;
        bool editing = false;
        bool set_focus = false;
        int64_t id = 0;
        int64_t request_id = 0;
        int64_t order_by = 0;
        std::string key;
        std::string value;
        std::string description;
    };

    struct AvRequestHeader : public AvRequestParam
    {
    };

    struct AvRequestCookie : public AvRequestParam
    {
    };

    struct AvEnvironmentVariable
    {
        int64_t id = 0;
        int64_t EnvId = 0;
        std::string key;
        std::string value;
    };

    struct AvEnvironment
    {
        int64_t id = 0;
        std::string name;
        std::vector<AvEnvironmentVariable> vars;
    };

    /// @brief One user-created request entry (the app-side model the UI lists,
    ///        selects and displays; sending it is wired up later).
    struct AvRequest
    {
        int64_t id = 0;
        int64_t timestamp = 0;
        int64_t order_by = 0;
        avNet::request_method method = avNet::request_method::get;
        std::string url = "https://jsonplaceholder.typicode.com/todos";
        std::vector<AvRequestParam> params;
        std::vector<AvRequestHeader> headers;
        std::vector<AvRequestCookie> cookies;
        std::optional<std::string> body;
        std::optional<std::string> title;
        std::optional<int> status_code;
        std::optional<std::string> collection;
        const std::string display_name() const { return title.value_or("request#" + std::to_string(id)); }

        avNet::http_request last_request;
        avNet::http_result last_result;
        bool pending_save = false;

        // --- history (recent sends) ------------------------------------------------
        // A snapshot is a fully independent, read-only record of one past send. It
        // shares no storage with the saved request it came from and it deliberately
        // carries `id == 0`: keeping the origin's id is what made an edit to a
        // snapshot upsert/delete the saved request's DB row, made the DB reload in
        // on_display_request_change overwrite the snapshot's captured rows, and made
        // origin + snapshots all highlight as selected at once.
        bool is_recent = false;
        /// unique per snapshot (see AvInterViewSharedState::recent_req_seq); 0 for a saved request
        int64_t snapshot_id = 0;
        /// id of the saved request this snapshot was taken from; 0 for a saved request.
        /// For display / grouping only - never used to read or write the DB.
        int64_t snapshot_of = 0;
        /// Snapshots of THIS request, oldest first. shared_ptr (not by value) so pushing
        /// a new snapshot never reallocates the ones already handed out as display_request,
        /// and so a selected snapshot can be kept alive while history is trimmed.
        std::vector<std::shared_ptr<AvRequest>> recent_reqs;
    };

    /// @brief Deep, self-contained snapshot of @p src for the history tree.
    ///        Deliberately NOT the implicit copy ctor: `recent_reqs` holds shared_ptr,
    ///        so a plain copy would alias the origin's snapshot list *and* nest it
    ///        (snapshot #N embedding #1..#N-1). Everything the snapshot owns is copied
    ///        by value; every link back to the origin's persisted rows is dropped.
    /// @param src the saved request being sent
    /// @param snapshot_id unique id from AvInterViewSharedState::recent_req_seq
    inline std::shared_ptr<AvRequest> make_snapshot(const AvRequest &src, int64_t snapshot_id)
    {
        std::shared_ptr<AvRequest> snap = std::make_shared<AvRequest>();

        // identity: the snapshot is nobody's DB row.
        snap->id = 0;
        snap->is_recent = true;
        snap->snapshot_id = snapshot_id;
        snap->snapshot_of = src.id;

        // payload, by value - later edits to the origin cannot reach it.
        snap->timestamp = src.timestamp;
        snap->order_by = src.order_by;
        snap->method = src.method;
        snap->url = src.url;
        snap->body = src.body;
        snap->status_code = src.status_code;
        snap->collection = src.collection;
        // resolve the name now: display_name() would fall back to "request#0" for a
        // snapshot, and the name as it was at send time is the honest thing to show.
        snap->title = src.display_name();

        snap->params = src.params;
        snap->headers = src.headers;
        snap->cookies = src.cookies;

        // The row ids belong to the origin's DB rows, so they cannot be kept. They are
        // replaced with *distinct negative* synthetic ids rather than 0: SQLite rowids are
        // always positive, so a negative id can never address a persisted row, while staying
        // unique keeps the id-based row erase in the editor from removing every row at once
        // (which is what a shared id of 0 would do).
        int64_t scratch_id = 0;
        for (AvRequestParam &p : snap->params)
        {
            p.id = --scratch_id;
            p.request_id = 0;
            p.editing = false;
            p.set_focus = false;
        }
        for (AvRequestHeader &h : snap->headers)
        {
            h.id = --scratch_id;
            h.request_id = 0;
            h.editing = false;
            h.set_focus = false;
        }
        for (AvRequestCookie &c : snap->cookies)
        {
            c.id = --scratch_id;
            c.request_id = 0;
            c.editing = false;
            c.set_focus = false;
        }

        // request/response are stamped by the sender; a snapshot never has snapshots.
        snap->last_request = {};
        snap->last_result = {};
        snap->pending_save = false;
        snap->recent_reqs.clear();

        return snap;
    }

} // namespace avR
