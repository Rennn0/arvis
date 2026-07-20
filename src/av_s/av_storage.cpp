#include <av_s/av_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvStorage::AvStorage()
        : db(std::make_unique<SQLite::Database>("_av_.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
    {
    }

    AvStorage::~AvStorage()
    {
    }
} // namespace avS
