#ifndef IIRFILTER_H
#define IIRFILTER_H

#include <thread>
#include <atomic>
#include <functional>
#include "DataBuffer.h"

class IIRFilter
{
public:
    enum class Algorithm
    {
        Exponential
    };

    using OutputCallback = std::function<void(const DataPoint &)>;

    IIRFilter();
    ~IIRFilter();

    void start(DataBuffer *inputBuffer);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setAlpha(float alpha);
    void setAlgorithm(Algorithm algo);
    void setOutputCallback(OutputCallback callback);
    void resetState();

    Algorithm getAlgorithm() const { return m_algorithm; }

private:
    void filterLoop();

    float exponentialFilter(float input);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    DataBuffer *m_inputBuffer{nullptr};
    Algorithm m_algorithm{Algorithm::Exponential};

    float m_alpha{0.3f};

    float m_prevOutput{0.0f};
    OutputCallback m_outputCallback;
};

#endif // IIRFILTER_H