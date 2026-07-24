#include <av_ui/ui_shortcut.hpp>

namespace avUi
{
    UiShortcut::UiShortcut()
    {
    }

    UiShortcut::~UiShortcut()
    {
    }
    UiShortcut::UiShortcut(std::string display, std::string binding, Predicate predicate, Callback callback)
        : display(std::move(display)), binding(std::move(binding)), predicate(std::move(predicate)),
          callback(std::move(callback))
    {
    }
    void UiShortcut::process() const
    {
        if (this->predicate && this->predicate() && this->callback)
            this->callback();

        if (this->shortcuts.size() > 0)
            for (const UiShortcut &s : this->shortcuts)
                s.process();
    }
    void UiShortcut::add(const UiShortcut shortcut)
    {
        this->shortcuts.push_back(shortcut);
    }
} // namespace avUi
