#pragma once

#include <functional>
#include <vector>
#include <string>

namespace avUi
{
    class UiShortcut
    {
    public:
        using Predicate = std::function<bool()>;
        using Callback = std::function<void()>;

        std::string display;
        std::string binding;
        std::vector<UiShortcut> shortcuts;
    public:
        UiShortcut();
        ~UiShortcut();

        UiShortcut(std::string display, std::string binding, Predicate predicate, Callback callback);
        void process() const;
        void add(const UiShortcut shortcut);

    private:
        Predicate predicate;
        Callback callback;
    };
} // namespace avUi
