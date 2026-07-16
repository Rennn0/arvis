#include <av_ui/network_manager_ui.hpp>

int main()
{
    const avUi::NetworkManagerUi *nmUi = new avUi::NetworkManagerUi();
    nmUi->start();
}
