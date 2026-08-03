#pragma once

namespace avR
{
    class AvAppSettings
    {
    public:
        bool save_responses = true;
        bool restore_last_req = true;

    public:
        AvAppSettings();
        ~AvAppSettings();

    private:
    };
} // namespace avR
