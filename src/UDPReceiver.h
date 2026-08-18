#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include "DataBuffer.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
#define CLOSE_SOCKET(s) ::closesocket(s)
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SET_NONBLOCKING(s)  \
    unsigned long mode = 1; \
    ioctlsocket(s, FIONBIO, &mode)
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
using SocketType = int;
#define CLOSE_SOCKET(s) ::close(s)
#define INVALID_SOCKET_VALUE -1
#define SET_NONBLOCKING(s)            \
    int flags = fcntl(s, F_GETFL, 0); \
    fcntl(s, F_SETFL, flags | O_NONBLOCK)
#endif

class UDPReceiver
{
public:
    using DataCallback = std::function<void(const DataPoint &)>;

    UDPReceiver();
    ~UDPReceiver();

    bool start(const std::string &bindAddr, int port);
    void stop();
    void setDataCallback(DataCallback callback);
    bool isRunning() const { return m_running.load(); }

private:
    void receiverLoop();
    void cleanup();

    SocketType m_socketFd{INVALID_SOCKET_VALUE};

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    struct sockaddr_in m_bindAddr{};
    DataCallback m_callback;
};

#endif