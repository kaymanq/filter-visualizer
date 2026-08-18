#ifndef FIRFILTER_H
#define FIRFILTER_H

#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <cstddef>
#include "DataBuffer.h"

class FIRFilter
{
public:
    enum class Algorithm
    {
        Boxcar,
        Hamming,
        Blackman,
        Median,
        Gaussian,
        LowPass
    };

    using OutputCallback = std::function<void(const DataPoint &)>;

    FIRFilter();
    ~FIRFilter();

    void start(DataBuffer *inputBuffer);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setWindowSize(size_t size);
    void setAlgorithm(Algorithm algo);
    void setCutoffFrequency(double freq);
    void setOutputCallback(OutputCallback callback);

    Algorithm getAlgorithm() const { return m_algorithm; }
    size_t getWindowSize() const { return m_windowSize; }

private:
    void filterLoop();
    void updateCoefficients();

    float boxcarFilter(const std::vector<float> &window) const;
    float hammingFilter(const std::vector<float> &window) const;
    float blackmanFilter(const std::vector<float> &window) const;
    float medianFilter(const std::vector<float> &window) const;
    float gaussianFilter(const std::vector<float> &window) const;
    float lowPassFilter(const std::vector<float> &window) const;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_windowSizeChanged{false};
    size_t m_newWindowSize{11};

    DataBuffer *m_inputBuffer{nullptr};
    size_t m_windowSize{11};
    Algorithm m_algorithm{Algorithm::Boxcar};
    double m_cutoffFreq{0.3};
    float m_invWindowSize{1.0f / 11.0f};

    std::vector<float> m_cachedCoeffs;
    size_t m_cachedWindowSize{0};
    Algorithm m_cachedAlgorithm{Algorithm::Boxcar};
    bool m_cachedCoeffsValid{false};

    OutputCallback m_outputCallback;
};

#endif