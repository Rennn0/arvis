#pragma once
/// @brief
class NetworkManager
{
public:
    /// @brief
    NetworkManager();

    /// @brief
    ~NetworkManager();

    /// @brief
    /// @param url
    void get(const char *url) const;

private:
    /// @brief
    /// @param str
    void print_info(const char *str) const;

    /// @brief
    /// @param str
    void print_error(const char *str) const;
};