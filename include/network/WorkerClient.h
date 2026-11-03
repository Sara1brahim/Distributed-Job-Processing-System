#ifndef WORKER_CLIENT_H
#define WORKER_CLIENT_H

#include <string>

class WorkerClient
{
private:
    int workerId;
    std::string serverAddress;
    int serverPort;

public:
    WorkerClient(
            int workerId,
            const std::string& serverAddress,
            int serverPort
    );

    void connectToCoordinator();
};

#endif