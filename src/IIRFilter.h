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
        Exponential,
        Butterworth
    };

    using OutputCallback = std::function<void(const DataPoint &)>;

    IIRFilter();
    ~IIRFilter();

    void start(DataBuffer *inputBuffer);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setAlpha(float alpha);
    void setCutoffFrequency(float freq);
    void setAlgorithm(Algorithm algo);
    void setOutputCallback(OutputCallback callback);
    void resetState();

    Algorithm getAlgorithm() const { return m_algorithm; }

private:
    void filterLoop();
    void updateButterworthCoeffs();

    float exponentialFilter(float input);
    float butterworthFilter(float input);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    DataBuffer *m_inputBuffer{nullptr};
    Algorithm m_algorithm{Algorithm::Exponential};

    float m_alpha{0.3f};
    float m_cutoffFreq{0.3f};

    double m_b0{1.0}, m_b1{2.0}, m_b2{1.0};
    double m_a1{1.414}, m_a2{1.0};

    struct ButterworthState
    {
        float x1{0.0f}, x2{0.0f};
        float y1{0.0f}, y2{0.0f};
    } m_butterworthState;

    float m_prevOutput{0.0f};
    OutputCallback m_outputCallback;
};

#endif