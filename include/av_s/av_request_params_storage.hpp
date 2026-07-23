#pragma once
#include <vector>
#include <av_s/av_storage.hpp>
#include <av_root/av_request.hpp>

namespace avS
{
    class AvRequestParamsStorage : private AvStorage
    {
    public:
        AvRequestParamsStorage();
        ~AvRequestParamsStorage();

        void del(int64_t id) const;
        void upsert(avR::AvRequestParams &requestParam) const;
        void upsert(std::vector<avR::AvRequestParams> &requestParams) const;
        std::vector<avR::AvRequestParams> select_by_req_id(int64_t requestId) const;

    private:
        const uint_fast8_t pcol_id = 1;
        const uint_fast8_t pcol_request_id = 2;
        const uint_fast8_t pcol_included = 3;
        const uint_fast8_t pcol_key = 4;
        const uint_fast8_t pcol_value = 5;
        const uint_fast8_t pcol_description = 6;
        const uint_fast8_t pcol_order_by = 7;

        const char *upsert_request_param_sql = "INSERT INTO request_params "
                                               "(id,request_id, included, rkey, rvalue, description, order_by) "
                                               "VALUES (?,?,?,?,?,?,?) "
                                               "ON CONFLICT(id) DO UPDATE SET "
                                               "request_id=excluded.request_id,"
                                               "included=excluded.included,"
                                               "rkey=excluded.rkey,"
                                               "rvalue=excluded.rvalue,"
                                               "description=excluded.description,"
                                               "order_by=excluded.order_by;";

        const char *create_request_params_table_sql =
            "CREATE TABLE IF NOT EXISTS request_params("
            "id INTEGER PRIMARY KEY,"
            "request_id INTEGER NOT NULL,"
            "included INTEGER NOT NULL DEFAULT 1,"
            "rkey TEXT,"
            "rvalue TEXT,"
            "description TEXT,"
            "order_by INTEGER NOT NULL DEFAULT 0,"
            "FOREIGN KEY(request_id) REFERENCES requests(id) ON DELETE CASCADE"
            ");";

        const char *select_all_request_param_sql =
            "SELECT * FROM request_params WHERE request_id = ? ORDER BY order_by;";
        const char *delete_request_param_sql = "DELETE FROM request_params WHERE id = ?;";
    };
} // namespace avS
