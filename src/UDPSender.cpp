#include "UDPSender.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

UDPSender::UDPSender()
{
    std::cout << "UDPSender: создан" << std::endl;
    m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socketFd < 0)
    {
        std::cerr << "Failed to create sender socket" << std::endl;
    }
}

UDPSender::~UDPSender()
{
    std::cout << "UDPSender: деструктор" << std::endl;
    close();
}

bool UDPSender::init(const std::string &targetIp, int targetPort)
{
    std::cout << "UDPSender::init: " << targetIp << ":" << targetPort << std::endl;

    if (m_socketFd < 0)
        return false;

    memset(&m_targetAddr, 0, sizeof(m_targetAddr));
    m_targetAddr.sin_family = AF_INET;
    m_targetAddr.sin_port = htons(targetPort);

    if (inet_pton(AF_INET, targetIp.c_str(), &m_targetAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid target IP: " << targetIp << std::endl;
        return false;
    }

    m_initialized = true;
    std::cout << "UDPSender::init: успешно" << std::endl;
    return true;
}

void UDPSender::sendTarget(float value)
{
    if (!m_initialized || m_socketFd < 0)
    {
        std::cerr << "UDPSender::sendTarget: не инициализирован" << std::endl;
        return;
    }

    std::cout << "UDPSender::sendTarget: отправка " << value << std::endl;
    sendto(m_socketFd, &value, sizeof(value), 0,
           (struct sockaddr *)&m_targetAddr, sizeof(m_targetAddr));
}

void UDPSender::close()
{
    std::cout << "UDPSender::close: начало" << std::endl;
    if (m_socketFd >= 0)
    {
#ifdef _WIN32
        closesocket(m_socketFd);
#else
        ::close(m_socketFd);
#endif
        m_socketFd = -1;
    }
    m_initialized = false;
    std::cout << "UDPSender::close: завершено" << std::endl;
}