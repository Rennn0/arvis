#include <av_s/av_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <fstream>
#include <cstdlib>

namespace avS
{

    std::filesystem::path get_path()
    {
        namespace fs = std::filesystem;
#if defined(_WIN32)
        const char *base = std::getenv("LOCALAPPDATA");
        fs::path dir = base ? fs::path(base) : fs::current_path();
#elif defined(__APPLE__)
        const char *home = std::getenv("HOME");
        fs::path dir = home ? fs::path(home) / "Library/Application Support" : fs::current_path();
#else
        const char *xdg = std::getenv("XDG_DATA_HOME");
        const char *home = std::getenv("HOME");
        fs::path dir = xdg ? fs::path(xdg) : (home ? fs::path(home) / ".local/share" : fs::current_path());
#endif
        dir /= "arvis";
        std::filesystem::create_directories(dir);
        return dir / "_av_.db";
    }

    AvStorage::AvStorage()
        : db(std::make_unique<SQLite::Database>(get_path().string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE))
    {
        this->db->exec("PRAGMA foreign_keys=ON;");
        this->db->exec("PRAGMA journal_mode=WAL;");
        this->db->exec("PRAGMA busy_timeout=3000;");
    }

    AvStorage::~AvStorage()
    {
    }

    int AvStorage::get_schema_version() const
    {
        SQLite::Statement q(*this->db.get(), "PRAGMA user_version");
        q.executeStep();

        return q.getColumn(0).getInt();
    }
    void AvStorage::set_schema_version(int version) const
    {
        this->db->exec("PRAGMA user_version=" + std::to_string(version));
    }
} // namespace avS
