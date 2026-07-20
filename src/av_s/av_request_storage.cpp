#include <av_s/av_request_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvRequestStorage::AvRequestStorage() : AvStorage()
    {
        this->db->exec(this->create_request_table_sql);
    }

    AvRequestStorage::~AvRequestStorage()
    {
    }

    void AvRequestStorage::upsert(avR::AvRequest *request) const
    {
        SQLite::Statement q(*this->db.get(), this->upsert_request_sql);
        bool isInsert = request->id == 0;
        if (isInsert)
            q.bind(this->col_id);
        else
            q.bind(this->col_id, request->id);

        q.bind(this->col_ts, request->timestamp);
        q.bind(this->col_method, static_cast<int>(request->method));
        q.bind(this->col_url, request->url);
        if (request->body)
            q.bind(this->col_body, request->body.value());
        else
            q.bind(this->col_body);
        if (request->title)
            q.bind(this->col_title, request->title.value());
        else
            q.bind(this->col_title);
        if (request->status_code)
            q.bind(this->col_status_code, request->status_code.value());
        else
            q.bind(this->col_status_code);
        if (request->collection)
            q.bind(this->col_collection, request->collection.value());
        else
            q.bind(this->col_collection);
        q.exec();

        if (isInsert)
            request->id = this->db->getLastInsertRowid();
    }

    void AvRequestStorage::upsert(std::vector<std::shared_ptr<avR::AvRequest>> &requests) const
    {
        SQLite::Transaction tran(*this->db.get());
        for (std::shared_ptr<avR::AvRequest> &req : requests)
            this->upsert(req.get());
        tran.commit();
    }

    std::vector<std::shared_ptr<avR::AvRequest>> AvRequestStorage::select_all() const
    {
        std::vector<std::shared_ptr<avR::AvRequest>> requests;
        SQLite::Statement q(*this->db.get(), this->select_all_request_sql);
        while (q.executeStep())
        {
            std::shared_ptr<avR::AvRequest> r = std::make_shared<avR::AvRequest>();
            r->id = q.getColumn(this->col_id - 1).getInt64();
            r->timestamp = q.getColumn(this->col_ts - 1).getInt64();
            r->method = static_cast<avNet::request_method>(q.getColumn(this->col_method - 1).getInt());
            r->url = q.getColumn(this->col_url - 1).getString();
            if (!q.getColumn(this->col_body - 1).isNull())
                r->body = q.getColumn(this->col_body - 1).getString();
            if (!q.getColumn(this->col_title - 1).isNull())
                r->title = q.getColumn(this->col_title - 1).getString();
            if (!q.getColumn(this->col_status_code - 1).isNull())
                r->status_code = q.getColumn(this->col_status_code - 1).getInt();
            if (!q.getColumn(this->col_collection - 1).isNull())
                r->collection = q.getColumn(this->col_collection - 1).getString();
            requests.push_back(std::move(r));
        }
        return requests;
    }

    void AvRequestStorage::del(int64_t id) const
    {
        SQLite::Statement q(*this->db.get(), this->delete_request_sql);
        q.bind(this->col_id, id);
        q.exec();
    }
} // namespace avS
