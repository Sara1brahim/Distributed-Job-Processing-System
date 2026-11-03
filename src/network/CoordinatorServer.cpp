#include "network/CoordinatorServer.h"
#include "network/NetworkUtils.h"

#include <iostream>
#include <thread>

CoordinatorServer::CoordinatorServer()
        : serverSocket(INVALID_SOCKET)
{
    jobs.push_back({101, 3});
    jobs.push_back({102, 2});
    jobs.push_back({103, 4});
    jobs.push_back({104, 2});
    jobs.push_back({105, 3});
}

bool CoordinatorServer::getNextJob(NetworkJob& job)
{
    std::lock_guard<std::mutex> lock(jobsMutex);

    if (jobs.empty())
        return false;

    job = jobs.front();
    jobs.erase(jobs.begin());

    return true;
}

void CoordinatorServer::start(int port)
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Failed to initialize Winsock.\n";
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create server socket.\n";
        WSACleanup();
        return;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(static_cast<u_short>(port));

    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
    ) == SOCKET_ERROR)
    {
        std::cerr << "Failed to bind server socket. Error: "
                  << WSAGetLastError() << '\n';

        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Failed to start listening.\n";

        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    std::cout << "Coordinator is listening on port "
              << port << "...\n";

    while (true)
    {
        SOCKET clientSocket = accept(
                serverSocket,
                nullptr,
                nullptr
        );

        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Failed to accept a worker connection.\n";
            continue;
        }

        std::cout << "A Worker connected!\n";

        std::thread(
                &CoordinatorServer::handleWorker,
                this,
                clientSocket
        ).detach();
    }
}

void CoordinatorServer::handleWorker(SOCKET clientSocket)
{
    std::string message;

    if (!receiveMessage(clientSocket, message))
    {
        closesocket(clientSocket);
        return;
    }

    if (message.rfind("REGISTER|", 0) != 0)
    {
        sendMessage(clientSocket, "ERROR|INVALID_REGISTRATION");
        closesocket(clientSocket);
        return;
    }

    std::string workerId = message.substr(9);

    std::cout << "Worker " << workerId
              << " registered successfully.\n";

    if (!sendMessage(clientSocket, "WELCOME|" + workerId))
    {
        closesocket(clientSocket);
        return;
    }

    while (receiveMessage(clientSocket, message))
    {
        if (message == "READY")
        {
            NetworkJob job;

            if (!getNextJob(job))
            {
                sendMessage(clientSocket, "ALL_DONE");
                break;
            }

            std::string jobMessage =
                    "JOB|" +
                    std::to_string(job.id) + "|" +
                    std::to_string(job.durationSeconds);

            std::cout << "Coordinator assigned Job "
                      << job.id << " to Worker "
                      << workerId << '\n';

            if (!sendMessage(clientSocket, jobMessage))
                break;
        }
        else if (message.rfind("RESULT|", 0) == 0)
        {
            std::cout << "Coordinator received from Worker "
                      << workerId << ": "
                      << message << '\n';
        }
        else
        {
            if (!sendMessage(clientSocket, "ERROR|UNKNOWN_MESSAGE"))
                break;
        }
    }

    std::cout << "Worker " << workerId
              << " disconnected.\n";

    closesocket(clientSocket);
}