#include <av_s/av_app_settings_storage.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

namespace avS
{
    AvAppSettingsStorage::AvAppSettingsStorage() : AvStorage()
    {
        this->db->exec(this->_create_app_settings_table_sql);
    }
    AvAppSettingsStorage::~AvAppSettingsStorage()
    {
    }
    void AvAppSettingsStorage::save(const avR::AvAppSettings *settings) const
    {
        using namespace SQLite;
        Transaction tran(*this->db.get());
        Statement q(*this->db, this->_upsert_app_setting_sql);
        const auto upsert = [this, &q](const char *key, const std::string &value)
        {
            q.reset();
            q.bind(this->_scol_key, key);
            q.bind(this->_scol_value, value);
            q.exec();
        };
        for (const BoolField &f : this->_bool_fields)
            upsert(f.key, settings->*f.member ? "1" : "0");

        for (const U8Field &f : this->_u8_fields)
            upsert(f.key, std::to_string(static_cast<int>(settings->*f.member)));

        tran.commit();
    }

    void AvAppSettingsStorage::load(avR::AvAppSettings *settings)
    {
        boost::container::flat_map<std::string, std::string> rows;
        using namespace SQLite;
        Statement q(*this->db, this->_select_all_app_settings_sql);
        while (q.executeStep())
            rows.emplace(q.getColumn(this->_scol_key - 1).getString(), q.getColumn(this->_scol_value - 1).getString());

        for (const BoolField &f : this->_bool_fields)
        {
            const auto it = rows.find(f.key);
            if (it != rows.end())
                settings->*f.member = this->parse_bool(it->second, settings->*f.member);
        }

        for (const U8Field &f : this->_u8_fields)
        {
            const auto it = rows.find(f.key);
            if (it != rows.end())
                settings->*f.member = this->parse_u8(it->second, settings->*f.member);
        }
    }
} // namespace avS
