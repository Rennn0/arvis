#include <av_s/av_request_cookies_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvRequestCookiesStorage::AvRequestCookiesStorage() : AvStorage()
    {
        this->db->exec(this->create_request_cookies_table_sql);
    }

    AvRequestCookiesStorage::~AvRequestCookiesStorage()
    {
    }

    void AvRequestCookiesStorage::del(int64_t id) const
    {
        SQLite::Statement q(*this->db, this->delete_request_cookie_sql);
        q.bind(this->ccol_id, id);
        q.exec();
    }
    void AvRequestCookiesStorage::upsert(avR::AvRequestCookie &requestCookie) const
    {
        SQLite::Statement q(*this->db, this->upsert_request_cookie_sql);
        bool isInsert = requestCookie.id == 0;
        if (isInsert)
            q.bind(this->ccol_id);
        else
            q.bind(this->ccol_id, requestCookie.id);

        q.bind(ccol_request_id, requestCookie.request_id);
        q.bind(ccol_included, requestCookie.included);
        q.bind(ccol_key, requestCookie.key);
        q.bind(ccol_value, requestCookie.value);
        q.bind(ccol_description, requestCookie.description);
        q.bind(ccol_order_by, requestCookie.order_by);
        q.exec();
        if (isInsert)
            requestCookie.id = this->db->getLastInsertRowid();
    }
    void AvRequestCookiesStorage::upsert(std::vector<avR::AvRequestCookie> &requestCookies) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (avR::AvRequestCookie &req : requestCookies)
            this->upsert(req);
        tran.commit();
    }

    std::vector<avR::AvRequestCookie> AvRequestCookiesStorage::select_by_req_id(int64_t requestId) const
    {
        std::vector<avR::AvRequestCookie> res;
        SQLite::Statement q(*this->db, this->select_all_request_cookie_sql);
        q.bind(1, requestId);
        while (q.executeStep())
        {
            avR::AvRequestCookie p;
            p.id = q.getColumn(this->ccol_id - 1).getInt64();
            p.request_id = q.getColumn(this->ccol_request_id - 1).getInt64();
            p.included = q.getColumn(this->ccol_included - 1).getInt() == 1;
            p.key = q.getColumn(this->ccol_key - 1).getText();
            p.value = q.getColumn(this->ccol_value - 1).getText();
            p.description = q.getColumn(this->ccol_description - 1).getText();
            p.order_by = q.getColumn(this->ccol_order_by - 1).getInt();
            res.push_back(p);
        }
        return res;
    }
} // namespace avS
