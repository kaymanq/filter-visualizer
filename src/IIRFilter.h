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
        Butterworth,
        Chebyshev,
        Bessel
    };

    using OutputCallback = std::function<void(const DataPoint &)>;

    IIRFilter();
    ~IIRFilter();

    void start(DataBuffer *inputBuffer);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setAlpha(float alpha);
    void setOrder(int order);
    void setCutoffFrequency(float freq);
    void setAlgorithm(Algorithm algo);
    void setOutputCallback(OutputCallback callback);

    Algorithm getAlgorithm() const { return m_algorithm; }

private:
    void filterLoop();

    float exponentialFilter(float input);
    float butterworthFilter(float input);
    float chebyshevFilter(float input);
    float besselFilter(float input);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    DataBuffer *m_inputBuffer{nullptr};
    Algorithm m_algorithm{Algorithm::Exponential};

    float m_alpha{0.3f};
    int m_order{2};
    float m_cutoffFreq{0.3f};

    struct ButterworthState
    {
        float x1{0.0f}, x2{0.0f};
        float y1{0.0f}, y2{0.0f};
    } m_butterworthState;

    struct ChebyshevState
    {
        float x1{0.0f}, x2{0.0f};
        float y1{0.0f}, y2{0.0f};
    } m_chebyshevState;

    struct BesselState
    {
        float x1{0.0f}, x2{0.0f};
        float y1{0.0f}, y2{0.0f};
    } m_besselState;

    float m_prevOutput{0.0f};
    OutputCallback m_outputCallback;
};

#endif // IIRFILTER_H