#pragma once
#include <iostream>
#include <string>
#include <string_view>

namespace avR
{
    class AvRoot
    {
    public:
        explicit AvRoot(std::string id);
        ~AvRoot();

        void log_info(const std::string_view msg) const noexcept;
        void log_error(const std::string_view msg) const noexcept;
        const std::string &get_id() const noexcept;

        bool is_today(int64_t ts) const;
        std::string timestamp_to_date(int64_t ts) const;
        const int64_t get_timestamp() const;

    private:
        void log_core(std::ostream &out, std::string_view level, std::string_view msg) const noexcept;
        std::string id;
    };
} // namespace avR
