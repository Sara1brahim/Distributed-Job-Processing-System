#include "network/WorkerClient.h"
#include "network/NetworkUtils.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

WorkerClient::WorkerClient(
        int workerId,
        const std::string& serverAddress,
        int serverPort
)
        : workerId(workerId),
          serverAddress(serverAddress),
          serverPort(serverPort)
{
}

void WorkerClient::connectToCoordinator()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Failed to initialize Winsock.\n";
        return;
    }

    SOCKET clientSocket = socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
    );

    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create Worker socket.\n";
        WSACleanup();
        return;
    }

    sockaddr_in serverAddressInfo{};

    serverAddressInfo.sin_family = AF_INET;
    serverAddressInfo.sin_port =
            htons(static_cast<u_short>(serverPort));

    if (inet_pton(
            AF_INET,
            serverAddress.c_str(),
            &serverAddressInfo.sin_addr
    ) != 1)
    {
        std::cerr << "Invalid Coordinator IP address.\n";

        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    if (connect(
            clientSocket,
            reinterpret_cast<sockaddr*>(&serverAddressInfo),
            sizeof(serverAddressInfo)
    ) == SOCKET_ERROR)
    {
        std::cerr << "Could not connect to Coordinator. Error: "
                  << WSAGetLastError() << '\n';

        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::cout << "Worker " << workerId
              << " connected to Coordinator.\n";

    if (!sendMessage(
            clientSocket,
            "REGISTER|" + std::to_string(workerId)
    ))
    {
        std::cerr << "Failed to register Worker.\n";
    }
    else
    {
        std::string response;

        if (!receiveMessage(clientSocket, response))
        {
            std::cerr << "No registration response received.\n";
        }
        else if (response != "WELCOME|" + std::to_string(workerId))
        {
            std::cerr << "Registration rejected: "
                      << response << '\n';
        }
        else
        {
            std::cout << "Coordinator response: "
                      << response << '\n';

            while (true)
            {
                if (!sendMessage(clientSocket, "READY"))
                {
                    std::cerr << "Failed to request a Job.\n";
                    break;
                }

                if (!receiveMessage(clientSocket, response))
                {
                    std::cerr << "Coordinator connection lost.\n";
                    break;
                }

                if (response == "ALL_DONE")
                {
                    std::cout << "No more Jobs available. "
                              << "Worker " << workerId
                              << " is shutting down.\n";
                    break;
                }

                if (response.rfind("JOB|", 0) != 0)
                {
                    std::cerr << "Invalid Coordinator response: "
                              << response << '\n';
                    break;
                }

                size_t firstSeparator = response.find('|');
                size_t secondSeparator =
                        response.find('|', firstSeparator + 1);

                if (secondSeparator == std::string::npos)
                {
                    std::cerr << "Invalid Job message.\n";
                    break;
                }

                int jobId = std::stoi(
                        response.substr(
                                firstSeparator + 1,
                                secondSeparator - firstSeparator - 1
                        )
                );

                int duration = std::stoi(
                        response.substr(secondSeparator + 1)
                );

                std::cout << "Worker " << workerId
                          << " started Job " << jobId
                          << '\n';

                std::this_thread::sleep_for(
                        std::chrono::seconds(duration)
                );

                std::cout << "Worker " << workerId
                          << " completed Job " << jobId
                          << '\n';

                std::string result =
                        "RESULT|" + std::to_string(jobId) + "|SUCCESS";

                if (!sendMessage(clientSocket, result))
                {
                    std::cerr << "Failed to report Job result.\n";
                    break;
                }
            }
        }
    }

    closesocket(clientSocket);
    WSACleanup();
}