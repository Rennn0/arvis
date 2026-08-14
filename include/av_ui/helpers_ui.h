#pragma once
#include <imgui.h>

constexpr ImVec4 Rgb(unsigned int hex, float a = 1.0f) {
    return ImVec4(((hex >> 16) & 0xFF) / 255.0f,
                  ((hex >>  8) & 0xFF) / 255.0f,
                  ( hex        & 0xFF) / 255.0f,
                  a);
}
