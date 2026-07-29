#include <av_root/root.hpp>
#include <chrono>
#include <ctime>
#include <cctype>
namespace avR
{
    AvRoot::AvRoot(std::string id) : id(std::move(id))
    {
    }

    AvRoot::~AvRoot()
    {
    }

    void AvRoot::log_info(const std::string_view msg) const noexcept
    {
        this->log_core(std::cout, "info", msg);
    }

    void AvRoot::log_error(const std::string_view msg) const noexcept
    {
        this->log_core(std::cerr, "error", msg);
    }

    void AvRoot::log_core(std::ostream &out, std::string_view level, std::string_view msg) const noexcept
    {
        out << '[' << level << ']' << '(' << this->id << ')' << msg << std::endl;
    }

    const std::string &AvRoot::get_id() const noexcept
    {
        return this->id;
    }

    bool AvRoot::is_today(int64_t ts) const
    {
        using namespace std::chrono;
        const auto tp = sys_time<milliseconds>{milliseconds{ts}};
        const auto req_day = floor<days>(tp);
        const auto today = floor<days>(system_clock::now());
        return req_day == today;
    }

    std::string AvRoot::timestamp_to_date(int64_t ts) const
    {
        using namespace std::chrono;
        sys_time<milliseconds> tp{milliseconds{ts}};
        std::time_t tt = system_clock::to_time_t(tp);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        char buf[40];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    const int64_t AvRoot::get_timestamp() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
} // namespace avR
