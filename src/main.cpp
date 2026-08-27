#include <av_ui/network_manager_ui.hpp>

int main()
{
    std::unique_ptr<avUi::NetworkManagerUi> app = std::make_unique<avUi::NetworkManagerUi>();
    app->run();
    return 0;
}
