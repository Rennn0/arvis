#pragma once
#include <cstdint>
namespace avR
{
    class AvAppSettings
    {
    public:
        bool _save_responses = true;
        bool _restore_last_req = false;
        bool _auto_save = false;

        uint8_t _active_theme_id = 0;
        uint8_t _active_font_id = 0;

    public:
        AvAppSettings();
        ~AvAppSettings();

    private:
    };
} // namespace avR
