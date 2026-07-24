#include <av_root/av_inter_view_shared_state.hpp>

namespace avR
{
    AvInterViewSharedState::AvInterViewSharedState()
        : show_req_list_view(true), show_req_detailed_view(true), display_request(nullptr), shortcut({})
    {
    }

    AvInterViewSharedState::~AvInterViewSharedState()
    {
    }
} // namespace avR
