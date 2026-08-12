#include "UDPReceiver.h"

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

UDPReceiver::UDPReceiver()
{
    std::cout << "UDPReceiver: создан" << std::endl;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

UDPReceiver::~UDPReceiver()
{
    std::cout << "UDPReceiver: деструктор" << std::endl;
    stop();
    if (m_socketFd >= 0)
    {
#ifdef _WIN32
        closesocket(m_socketFd);
#else
        close(m_socketFd);
#endif
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

bool UDPReceiver::start(const std::string &bindAddr, int port)
{
    std::cout << "UDPReceiver::start: начало" << std::endl;

    if (m_running.load())
    {
        std::cout << "UDPReceiver::start: уже запущен" << std::endl;
        return false;
    }

    std::cout << "UDPReceiver::start: создаем сокет..." << std::endl;
    m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socketFd < 0)
    {
        std::cerr << "Failed to create UDP socket" << std::endl;
        return false;
    }

    memset(&m_bindAddr, 0, sizeof(m_bindAddr));
    m_bindAddr.sin_family = AF_INET;
    m_bindAddr.sin_port = htons(port);

    if (bindAddr.empty() || bindAddr == "0.0.0.0")
    {
        m_bindAddr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (inet_pton(AF_INET, bindAddr.c_str(), &m_bindAddr.sin_addr) <= 0)
        {
            std::cerr << "Invalid IP address: " << bindAddr << std::endl;
#ifdef _WIN32
            closesocket(m_socketFd);
#else
            close(m_socketFd);
#endif
            m_socketFd = -1;
            return false;
        }
    }

    std::cout << "UDPReceiver::start: биндим сокет на порт " << port << "..." << std::endl;
    if (bind(m_socketFd, (struct sockaddr *)&m_bindAddr, sizeof(m_bindAddr)) < 0)
    {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
#ifdef _WIN32
        closesocket(m_socketFd);
#else
        close(m_socketFd);
#endif
        m_socketFd = -1;
        return false;
    }

    std::cout << "UDPReceiver::start: запускаем поток..." << std::endl;
    m_running.store(true);
    m_stopRequested.store(false);
    m_thread = std::thread(&UDPReceiver::receiverLoop, this);

    std::cout << "UDPReceiver::start: поток запущен, выходим" << std::endl;
    return true;
}

void UDPReceiver::stop()
{
    std::cout << "UDPReceiver::stop: начало" << std::endl;

    if (m_running.load())
    {
        m_stopRequested.store(true);
        m_running.store(false);
        if (m_thread.joinable())
        {
            std::cout << "UDPReceiver::stop: ждем завершения потока..." << std::endl;
            m_thread.join();
            std::cout << "UDPReceiver::stop: поток завершен" << std::endl;
        }
        if (m_socketFd >= 0)
        {
#ifdef _WIN32
            closesocket(m_socketFd);
#else
            close(m_socketFd);
#endif
            m_socketFd = -1;
        }
    }

    std::cout << "UDPReceiver::stop: завершено" << std::endl;
}

void UDPReceiver::setDataCallback(DataCallback callback)
{
    m_callback = callback;
}

void UDPReceiver::receiverLoop()
{
    std::cout << "UDPReceiver::receiverLoop: поток запущен" << std::endl;

    struct sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    char buffer[64];

    while (!m_stopRequested.load())
    {
        ssize_t recvLen = recvfrom(m_socketFd, buffer, sizeof(buffer), 0,
                                   (struct sockaddr *)&clientAddr, &clientLen);

        if (recvLen < 0)
        {
            if (m_stopRequested.load())
                break;
            std::cerr << "recvfrom error" << std::endl;
            continue;
        }

        if (recvLen >= sizeof(uint32_t) + sizeof(float))
        {
            DataPoint point;
            memcpy(&point.timestamp, buffer, sizeof(uint32_t));
            memcpy(&point.value, buffer + sizeof(uint32_t), sizeof(float));

            if (m_callback)
            {
                m_callback(point);
            }
        }
    }

    std::cout << "UDPReceiver::receiverLoop: поток завершен" << std::endl;
}