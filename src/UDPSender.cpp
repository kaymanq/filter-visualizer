#include "UDPSender.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
#define GET_LAST_ERROR() WSAGetLastError()
#else
#define GET_LAST_ERROR() errno
#include <errno.h>
#endif

UDPSender::UDPSender()
{
    std::cout << "UDPSender: создан" << std::endl;
    m_socketFd = INVALID_SOCKET_VALUE;
}

UDPSender::~UDPSender()
{
    std::cout << "UDPSender: деструктор" << std::endl;
    closeSocket();
}

bool UDPSender::init(const std::string &targetIp, int targetPort)
{
    std::cout << "UDPSender::init: " << targetIp << ":" << targetPort << std::endl;

    // Закрываем старый сокет, если он был открыт
    if (m_socketFd != INVALID_SOCKET_VALUE)
    {
        CLOSE_SOCKET(m_socketFd);
        m_socketFd = INVALID_SOCKET_VALUE;
    }

    m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socketFd == INVALID_SOCKET_VALUE)
    {
        std::cerr << "Failed to create sender socket. Error: " << GET_LAST_ERROR() << std::endl;
        return false;
    }

    memset(&m_targetAddr, 0, sizeof(m_targetAddr));
    m_targetAddr.sin_family = AF_INET;
    m_targetAddr.sin_port = htons(targetPort);

    if (inet_pton(AF_INET, targetIp.c_str(), &m_targetAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid target IP: " << targetIp << std::endl;
        CLOSE_SOCKET(m_socketFd);
        m_socketFd = INVALID_SOCKET_VALUE;
        return false;
    }

    m_initialized = true;
    std::cout << "UDPSender::init: успешно" << std::endl;
    return true;
}

void UDPSender::sendTarget(float value)
{
    if (!m_initialized || m_socketFd == INVALID_SOCKET_VALUE)
    {
        std::cerr << "UDPSender::sendTarget: не инициализирован" << std::endl;
        return;
    }

    int sent = sendto(
        m_socketFd,
        (const char *)&value,
        sizeof(value),
        0,
        (struct sockaddr *)&m_targetAddr,
        sizeof(m_targetAddr));

    if (sent < 0)
    {
        std::cerr << "UDPSender::sendTarget: ошибка отправки. Error: " << GET_LAST_ERROR() << std::endl;
    }
    else
    {
        std::cout << "UDPSender::sendTarget: отправлено " << value << std::endl;
    }
}

void UDPSender::closeSocket()
{
    std::cout << "UDPSender::closeSocket: начало" << std::endl;

    if (m_socketFd != INVALID_SOCKET_VALUE)
    {
        CLOSE_SOCKET(m_socketFd);
        m_socketFd = INVALID_SOCKET_VALUE;
    }

    m_initialized = false;
    std::cout << "UDPSender::closeSocket: завершено" << std::endl;
}