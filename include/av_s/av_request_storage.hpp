#pragma once
#include <vector>
#include <memory>
#include <av_s/av_storage.hpp>
#include <av_root/av_request.hpp>

namespace avS
{
    class AvRequestStorage : private AvStorage
    {
    public:
        AvRequestStorage();
        ~AvRequestStorage();

        void upsert(avR::AvRequest *request) const;
        void upsert(std::vector<std::shared_ptr<avR::AvRequest>> &requests) const;        
        std::vector<std::shared_ptr<avR::AvRequest>> select_all() const;
        void del(int64_t id) const;

    private:
        void migrate() const;

        void migrate_to_v1() const;
        void migrate_to_v2() const;

        const uint_fast8_t col_id = 1;
        const uint_fast8_t col_ts = 2;
        const uint_fast8_t col_method = 3;
        const uint_fast8_t col_url = 4;
        const uint_fast8_t col_body = 5;
        const uint_fast8_t col_title = 6;
        const uint_fast8_t col_status_code = 7;
        const uint_fast8_t col_collection = 8;
        const uint_fast8_t col_order_by = 9;


        const char *create_request_table_sql = "CREATE TABLE IF NOT EXISTS requests("
                                               "id INTEGER PRIMARY KEY,"
                                               "ts INTEGER NOT NULL,"
                                               "method INTEGER NOT NULL,"
                                               "url TEXT NOT NULL,"
                                               "body TEXT,"
                                               "title TEXT,"
                                               "status_code INTEGER,"
                                               "collection TEXT"
                                               ");";

        const char *upsert_request_sql = "INSERT INTO requests "
                                         "(id, ts, method, url, body, title, status_code, collection, order_by) "
                                         "VALUES (?,?,?,?,?,?,?,?,?) "
                                         "ON CONFLICT(id) DO UPDATE SET "
                                         "ts=excluded.ts,"
                                         "method=excluded.method,"
                                         "url=excluded.url,"
                                         "body=excluded.body,"
                                         "title=excluded.title,"
                                         "status_code=excluded.status_code,"
                                         "collection=excluded.collection,"
                                         "order_by=excluded.order_by;";

        const char *select_all_request_sql = "SELECT * FROM requests ORDER BY order_by;";
        const char *delete_request_sql = "DELETE FROM requests WHERE id = ?;";
    };

} // namespace avS
