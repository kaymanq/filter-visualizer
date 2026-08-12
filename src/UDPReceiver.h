#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <netinet/in.h>
#include "DataBuffer.h"

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

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    int m_socketFd{-1};
    struct sockaddr_in m_bindAddr{};
    DataCallback m_callback;
};

#endif // UDPRECEIVER_H