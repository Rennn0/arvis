#pragma once

#include <imgui.h>

namespace avR
{
    class UiScopedId
    {
    public:
        UiScopedId(const char *id);
        UiScopedId(int id);
        UiScopedId(const void* id);
        ~UiScopedId();

    private:
    };
} // namespace avR
