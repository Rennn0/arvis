#pragma once
#include <vector>
#include <av_s/av_storage.hpp>
#include <av_root/av_request.hpp>
namespace avS
{
    class AvRequestCookiesStorage : private AvStorage
    {
    public:
        AvRequestCookiesStorage();
        ~AvRequestCookiesStorage();

        void del(int64_t id) const;
        void upsert(avR::AvRequestCookie &requestCookie) const;
        void upsert(std::vector<avR::AvRequestCookie> &requestCookies) const;
        std::vector<avR::AvRequestCookie> select_by_req_id(int64_t requestId) const;

    private:
        const uint_fast8_t ccol_id = 1;
        const uint_fast8_t ccol_request_id = 2;
        const uint_fast8_t ccol_included = 3;
        const uint_fast8_t ccol_key = 4;
        const uint_fast8_t ccol_value = 5;
        const uint_fast8_t ccol_description = 6;
        const uint_fast8_t ccol_order_by = 7;

        const char *upsert_request_cookie_sql = "INSERT INTO request_cookies "
                                                "(id,request_id, included, rkey, rvalue, description, order_by) "
                                                "VALUES (?,?,?,?,?,?,?) "
                                                "ON CONFLICT(id) DO UPDATE SET "
                                                "request_id=excluded.request_id,"
                                                "included=excluded.included,"
                                                "rkey=excluded.rkey,"
                                                "rvalue=excluded.rvalue,"
                                                "description=excluded.description,"
                                                "order_by=excluded.order_by;";

        const char *create_request_cookies_table_sql =
            "CREATE TABLE IF NOT EXISTS request_cookies("
            "id INTEGER PRIMARY KEY,"
            "request_id INTEGER NOT NULL,"
            "included INTEGER NOT NULL DEFAULT 1,"
            "rkey TEXT,"
            "rvalue TEXT,"
            "description TEXT,"
            "order_by INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY(request_id) REFERENCES requests(id) ON DELETE CASCADE"
            ");";

        const char *select_all_request_cookie_sql =
            "SELECT * FROM request_cookies WHERE request_id = ? ORDER BY order_by;";
        const char *delete_request_cookie_sql = "DELETE FROM request_cookies WHERE id = ?;";
    };
} // namespace avS
