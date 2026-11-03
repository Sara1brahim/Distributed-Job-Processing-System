#include "network/WorkerClient.h"

#include <iostream>

int main(int argc, char* argv[])
{
    int workerId = 1;

    if (argc >= 2)
    {
        try
        {
            workerId = std::stoi(argv[1]);
        }
        catch (...)
        {
            std::cerr << "Invalid Worker ID.\n";
            return 1;
        }
    }

    WorkerClient worker(
            workerId,
            "127.0.0.1",
            54000
    );

    worker.connectToCoordinator();

    return 0;
}