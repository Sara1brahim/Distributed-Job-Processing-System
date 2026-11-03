#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <winsock2.h>
#include <string>

inline bool sendMessage(SOCKET socket, const std::string& message)
{
    std::string data = message + "\n";

    int totalSent = 0;
    int dataSize = static_cast<int>(data.size());

    while (totalSent < dataSize)
    {
        int sent = send(
                socket,
                data.c_str() + totalSent,
                dataSize - totalSent,
                0
        );

        if (sent == SOCKET_ERROR || sent == 0)
            return false;

        totalSent += sent;
    }

    return true;
}

inline bool receiveMessage(SOCKET socket, std::string& message)
{
    message.clear();

    char character;

    while (true)
    {
        int received = recv(socket, &character, 1, 0);

        if (received <= 0)
            return false;

        if (character == '\n')
            return true;

        message += character;
    }
}

#endif