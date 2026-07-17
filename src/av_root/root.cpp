#include <av_root/root.hpp>
namespace avR
{
    AvRoot::AvRoot(std::string name) : name(std::move(name))
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
        out << '[' << level << ']' << '(' << this->name << ')' << msg << std::endl;
    }

    const std::string &AvRoot::get_name() const noexcept
    {
        return this->name;
    }
} // namespace avR
