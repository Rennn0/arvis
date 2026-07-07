#include <network_manager.hpp>

int main()
{
    const NetworkManager *networkManagerPtr = new NetworkManager();
    networkManagerPtr->get("https://api.xati.org/health");
    networkManagerPtr->get("https://www.xati.org/18");

    delete networkManagerPtr;
    return 0;
}