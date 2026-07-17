#pragma once
#include <av_root/ui_container_component.hpp>
#include <vector>

namespace avR
{
    /// @brief Block container ("div"). By default it fills all the space available
    ///        in its parent and stacks its children. Intended to be the root:
    ///        create one, then add_child() buttons / inputs / other layouts into it.
    class AvDiv : public UiContainerComponent
    {
    public:
        explicit AvDiv(std::string id, Config config);

        /// @brief Replace the whole config; returns *this for chaining.
        AvDiv &configure(const Config &config);

        void set_layout_size(const ImVec2 &size) override;
        ImVec2 preferred_size() const override;

    protected:
        void render() override;

    private:
        void layout_children();

        Config config;
    };
} // namespace avR
