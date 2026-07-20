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
        std::unique_ptr<SQLite::Database> db;
    };
} // namespace avS
