#pragma once

#include <string>
#include <memory>
#include <av_root/av_request.hpp>


namespace SQLite
{
    class Database;
}

namespace avS
{
    class AvStorage
    {
    public:
        AvStorage();
        ~AvStorage();

    protected:
        const long appSchemaVersion = 2;
        std::unique_ptr<SQLite::Database> db;

        int get_schema_version() const;
        void set_schema_version(int version) const;
    };
} // namespace avS
