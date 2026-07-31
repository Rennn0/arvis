#pragma once

#include <av_root/ui_component.hpp>
#include <av_root/av_inter_view_shared_state.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace avUi
{
    class SearchViewUi : public avR::UiComponent
    {
    public:
        SearchViewUi() = delete;
        ~SearchViewUi();

        explicit SearchViewUi(std::string id, avR::AvState *sharedState);

    private:
        avR::AvInterViewSharedState *shared_state;

        void render() override;

        std::optional<std::function<void(const avR::AvRequest *)>> on_pick;

        // one searchable field: ":t" / ":title" -> AvRequest::display_name()
        struct SearchField
        {
            char key = 0;                                               // short prefix char
            std::string name;                                           // long prefix name
            std::function<std::string(const avR::AvRequest &)> extract; // value the term is matched against
        };

        struct QueryTerm
        {
            const SearchField *field = nullptr;
            std::string text;
        };

        // NOTE: deliberately stores an id + copied strings, not a pointer into the request vector,
        // so a reallocation of the underlying list while the palette is open can't dangle.
        struct Hit
        {
            int64_t id = 0;
            int score = 0;
            std::string label;
            std::string sub;
            avNet::request_method method = avNet::request_method::get;
        };

        const SearchField *field_for(const std::string &key) const;
        std::vector<QueryTerm> parse_query(const std::string &raw) const;

        void rebuild();
        void move_selection(int delta);
        void accept();
        void open_reset();
        void close();

        std::vector<SearchField> fields;
        std::vector<Hit> hits;

        char query[256] = {};
        int selected = 0;
        int open_frames = 0;
        bool dirty = true;
        bool want_focus = false;
        bool want_scroll = false;
        bool was_open = false;
    };
    
} // namespace avUi
