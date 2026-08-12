#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <string>
#include "UDPReceiver.h" // Для кроссплатформенных определений

class UDPSender
{
public:
    UDPSender();
    ~UDPSender();

    bool init(const std::string &targetIp, int targetPort);
    void sendTarget(float value);
    void closeSocket(); // Переименовано, чтобы не конфликтовать

private:
    SocketType m_socketFd{INVALID_SOCKET_VALUE};
    struct sockaddr_in m_targetAddr{};
    bool m_initialized{false};
};

#endif // UDPSENDER_H