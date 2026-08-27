#pragma once
#include <av_s/av_storage.hpp>
#include <av_root/av_app_settings.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/container/flat_map.hpp>
namespace avS
{
    class AvAppSettingsStorage : private AvStorage
    {
    public:
        AvAppSettingsStorage();
        ~AvAppSettingsStorage();

        using AvStorage::get_db_path;

        void save(const avR::AvAppSettings *settings) const;
        void load(avR::AvAppSettings *settings);

    private:
        const uint_fast8_t _scol_key = 1;
        const uint_fast8_t _scol_value = 2;

        const char *_create_app_settings_table_sql = "CREATE TABLE IF NOT EXISTS app_settings("
                                                     "skey TEXT PRIMARY KEY,"
                                                     "svalue TEXT NOT NULL"
                                                     ");";

        const char *_upsert_app_setting_sql = "INSERT INTO app_settings "
                                              "(skey, svalue) "
                                              "VALUES (?,?) "
                                              "ON CONFLICT(skey) DO UPDATE SET "
                                              "svalue=excluded.svalue;";

        const char *_select_all_app_settings_sql = "SELECT * FROM app_settings;";

        using Settings = avR::AvAppSettings;

        struct BoolField
        {
            const char *key;
            bool Settings::*member;
        };

        struct U8Field
        {
            const char *key;
            uint8_t Settings::*member;
        };

        const BoolField _bool_fields[3] = {
            {"save_responses", &Settings::_save_responses},
            {"restore_last_req", &Settings::_restore_last_req},
            {"auto_save", &Settings::_auto_save},
        };

        const U8Field _u8_fields[2] = {
            {"active_theme_id", &Settings::_active_theme_id},
            {"active_font_id", &Settings::_active_font_id},
        };

        bool parse_bool(std::string raw, bool fallback)
        {
            boost::trim(raw);
            if (raw.empty())
                return fallback;
            return raw == "1" || boost::iequals(raw, "true") || boost::iequals(raw, "yes");
        }

        uint8_t parse_u8(std::string raw, uint8_t fallback)
        {
            boost::trim(raw);
            int parsed = 0;
            const char *first = raw.data();
            const char *last = raw.data() + raw.size();
            const std::from_chars_result r = std::from_chars(first, last, parsed);
            if (r.ec != std::errc{} || r.ptr != last)
                return fallback;

            return static_cast<uint8_t>(std::clamp(parsed, 0, 255));
        }
    };
} // namespace avS
