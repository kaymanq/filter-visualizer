#include "UDPReceiver.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
#define GET_LAST_ERROR() WSAGetLastError()
#define ERRNO_IS_WOULDBLOCK() (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#define GET_LAST_ERROR() errno
#define ERRNO_IS_WOULDBLOCK() (errno == EAGAIN || errno == EWOULDBLOCK)
#endif

UDPReceiver::UDPReceiver()
{
    std::cout << "UDPReceiver: создан" << std::endl;

#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
#endif
}

UDPReceiver::~UDPReceiver()
{
    std::cout << "UDPReceiver: деструктор" << std::endl;
    stop();
    cleanup();

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

    if (m_socketFd == INVALID_SOCKET_VALUE)
    {
        std::cerr << "Failed to create UDP socket. Error: " << GET_LAST_ERROR() << std::endl;
        return false;
    }

    SET_NONBLOCKING(m_socketFd);

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
            CLOSE_SOCKET(m_socketFd);
            m_socketFd = INVALID_SOCKET_VALUE;
            return false;
        }
    }

    std::cout << "UDPReceiver::start: биндим сокет на порт " << port << "..." << std::endl;
    if (bind(m_socketFd, (struct sockaddr *)&m_bindAddr, sizeof(m_bindAddr)) < 0)
    {
        std::cerr << "Failed to bind socket. Error: " << GET_LAST_ERROR() << std::endl;
        CLOSE_SOCKET(m_socketFd);
        m_socketFd = INVALID_SOCKET_VALUE;
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

        if (m_socketFd != INVALID_SOCKET_VALUE)
        {
            CLOSE_SOCKET(m_socketFd);
            m_socketFd = INVALID_SOCKET_VALUE;
        }

        if (m_thread.joinable())
        {
            std::cout << "UDPReceiver::stop: ждем завершения потока..." << std::endl;
            m_thread.join();
            std::cout << "UDPReceiver::stop: поток завершен" << std::endl;
        }
    }

    std::cout << "UDPReceiver::stop: завершено" << std::endl;
}

void UDPReceiver::cleanup()
{
    if (m_socketFd != INVALID_SOCKET_VALUE)
    {
        CLOSE_SOCKET(m_socketFd);
        m_socketFd = INVALID_SOCKET_VALUE;
    }
}

void UDPReceiver::setDataCallback(DataCallback callback)
{
    m_callback = callback;
}

void UDPReceiver::receiverLoop()
{
    std::cout << "UDPReceiver::receiverLoop: поток запущен" << std::endl;

    char buffer[64];
    struct sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    while (!m_stopRequested.load())
    {
        int recvLen = recvfrom(
            m_socketFd,
            buffer,
            sizeof(buffer),
            0,
            (struct sockaddr *)&clientAddr,
            &clientLen);

        if (recvLen < 0)
        {
            if (m_stopRequested.load())
            {
                std::cout << "UDPReceiver::receiverLoop: получен сигнал остановки" << std::endl;
                break;
            }

            if (!ERRNO_IS_WOULDBLOCK())
            {
                std::cerr << "recvfrom error: " << GET_LAST_ERROR() << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (recvLen >= sizeof(uint32_t) + sizeof(float))
        {
            DataPoint point;
            memcpy(&point.timestamp, buffer, sizeof(uint32_t));
            memcpy(&point.value, buffer + sizeof(uint32_t), sizeof(float));

            static int counter = 0;
            if (++counter % 100 == 0)
            {
                std::cout << "UDPReceiver: получено " << counter
                          << " пакетов, значение=" << point.value << std::endl;
            }

            if (m_callback)
            {
                m_callback(point);
            }
        }
    }

    std::cout << "UDPReceiver::receiverLoop: поток завершен" << std::endl;
}