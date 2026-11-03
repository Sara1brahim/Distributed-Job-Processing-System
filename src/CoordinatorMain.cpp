#include "network/CoordinatorServer.h"

int main()
{
    CoordinatorServer coordinator;

    coordinator.start(54000);

    return 0;
}