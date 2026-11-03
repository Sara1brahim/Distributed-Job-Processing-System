#ifndef COORDINATOR_SERVER_H
#define COORDINATOR_SERVER_H

#include <winsock2.h>
#include <string>
#include <vector>
#include <mutex>

struct NetworkJob
{
    int id;
    int durationSeconds;
};

class CoordinatorServer
{
private:
    SOCKET serverSocket;

    std::vector<NetworkJob> jobs;
    std::mutex jobsMutex;

    void handleWorker(SOCKET clientSocket);

    bool getNextJob(NetworkJob& job);

public:
    CoordinatorServer();

    CoordinatorServer(const CoordinatorServer&) = delete;
    CoordinatorServer& operator=(const CoordinatorServer&) = delete;

    void start(int port);
};

#endif