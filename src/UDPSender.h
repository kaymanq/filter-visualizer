#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <string>
#include "UDPReceiver.h"

class UDPSender
{
public:
    UDPSender();
    ~UDPSender();

    bool init(const std::string &targetIp, int targetPort);
    void sendTarget(float value);
    void closeSocket();

private:
    SocketType m_socketFd{INVALID_SOCKET_VALUE};
    struct sockaddr_in m_targetAddr{};
    bool m_initialized{false};
};

#endif