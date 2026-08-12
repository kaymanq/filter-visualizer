#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <string>
#include <netinet/in.h>

class UDPSender
{
public:
    UDPSender();
    ~UDPSender();

    bool init(const std::string &targetIp, int targetPort);
    void sendTarget(float value);
    void close();

private:
    int m_socketFd{-1};
    struct sockaddr_in m_targetAddr{};
    bool m_initialized{false};
};

#endif // UDPSENDER_H