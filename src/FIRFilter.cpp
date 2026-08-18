#include "FIRFilter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template <typename T>
static T clamp(T value, T min, T max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

FIRFilter::FIRFilter()
{
    std::cout << "FIRFilter: создан" << std::endl;
}

FIRFilter::~FIRFilter()
{
    std::cout << "FIRFilter: деструктор" << std::endl;
    stop();
}

void FIRFilter::start(DataBuffer *inputBuffer)
{
    std::cout << "FIRFilter::start: начало" << std::endl;

    if (m_running.load())
    {
        std::cout << "FIRFilter::start: уже запущен" << std::endl;
        return;
    }

    m_inputBuffer = inputBuffer;
    m_running.store(true);
    m_stopRequested.store(false);

    std::cout << "FIRFilter::start: создаем поток" << std::endl;
    m_thread = std::thread(&FIRFilter::filterLoop, this);

    std::cout << "FIRFilter::start: поток создан, выходим" << std::endl;
}

void FIRFilter::stop()
{
    std::cout << "FIRFilter::stop: начало" << std::endl;

    if (m_running.load())
    {
        m_stopRequested.store(true);
        m_running.store(false);
        if (m_thread.joinable())
        {
            std::cout << "FIRFilter::stop: ждем завершения потока..." << std::endl;
            m_thread.join();
            std::cout << "FIRFilter::stop: поток завершен" << std::endl;
        }
    }

    std::cout << "FIRFilter::stop: завершено" << std::endl;
}

void FIRFilter::setWindowSize(size_t size)
{
    size_t newSize = (size % 2 == 0) ? size + 1 : size;
    if (newSize != m_windowSize)
    {
        m_newWindowSize = newSize;
        m_windowSizeChanged.store(true);
        std::cout << "FIRFilter: запрошено изменение размера окна на " << m_newWindowSize << std::endl;
    }
}

void FIRFilter::setAlgorithm(Algorithm algo)
{
    m_algorithm = algo;
    m_cachedCoeffsValid = false;
    std::cout << "FIRFilter: алгоритм изменен на ";
    switch (algo)
    {
    case Algorithm::Boxcar:
        std::cout << "Boxcar";
        break;
    case Algorithm::Hamming:
        std::cout << "Hamming";
        break;
    case Algorithm::Blackman:
        std::cout << "Blackman";
        break;
    case Algorithm::Median:
        std::cout << "Median";
        break;
    case Algorithm::Gaussian:
        std::cout << "Gaussian";
        break;
    case Algorithm::LowPass:
        std::cout << "LowPass";
        break;
    default:
        std::cout << "Unknown";
        break;
    }
    std::cout << std::endl;
    std::cout.flush();
}

void FIRFilter::setCutoffFrequency(double freq)
{
    m_cutoffFreq = clamp(freq, 0.01, 0.99);
    if (m_algorithm == Algorithm::LowPass)
    {
        m_cachedCoeffsValid = false;
    }
    std::cout << "FIRFilter: cutoff frequency = " << m_cutoffFreq << std::endl;
}

void FIRFilter::setOutputCallback(OutputCallback callback)
{
    m_outputCallback = callback;
}

void FIRFilter::updateCoefficients()
{
    if (m_cachedCoeffsValid &&
        m_cachedWindowSize == m_windowSize &&
        m_cachedAlgorithm == m_algorithm)
    {
        return;
    }

    m_cachedWindowSize = m_windowSize;
    m_cachedAlgorithm = m_algorithm;
    m_cachedCoeffs.clear();
    m_cachedCoeffs.reserve(m_windowSize);

    int N = static_cast<int>(m_windowSize);
    m_invWindowSize = 1.0f / static_cast<float>(N);

    switch (m_algorithm)
    {
    case Algorithm::Hamming:
        for (int i = 0; i < N; ++i)
        {
            double angle = 2.0 * M_PI * i / (N - 1);
            m_cachedCoeffs.push_back(0.54f - 0.46f * static_cast<float>(std::cos(angle)));
        }
        break;
    case Algorithm::Blackman:
        for (int i = 0; i < N; ++i)
        {
            double angle = 2.0 * M_PI * i / (N - 1);
            m_cachedCoeffs.push_back(0.42f - 0.5f * static_cast<float>(std::cos(angle)) + 0.08f * static_cast<float>(std::cos(2.0 * angle)));
        }
        break;
    case Algorithm::Gaussian:
    {
        float sigma = static_cast<float>(N) / 6.0f;
        int center = N / 2;
        m_cachedCoeffs.resize(N);
        for (int i = 0; i < N; ++i)
        {
            float x = static_cast<float>(i - center);
            m_cachedCoeffs[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        }
    }
    break;
    case Algorithm::LowPass:
    {
        float fc = static_cast<float>(m_cutoffFreq);
        float cutoff = fc * 2.0f;
        int center = N / 2;
        m_cachedCoeffs.resize(N);
        for (int i = 0; i < N; ++i)
        {
            float x = static_cast<float>(i - center);
            float sinc;
            if (std::abs(x) < 1e-6f)
            {
                sinc = cutoff;
            }
            else
            {
                double arg = M_PI * cutoff * x;
                sinc = static_cast<float>(cutoff * std::sin(arg) / (M_PI * cutoff * x));
            }
            double angle = 2.0 * M_PI * i / (N - 1);
            float hamming = 0.54f - 0.46f * static_cast<float>(std::cos(angle));
            m_cachedCoeffs[i] = sinc * hamming;
        }
    }
    break;
    default:
        m_cachedCoeffs.assign(N, m_invWindowSize);
        break;
    }

    m_cachedCoeffsValid = true;
}

void FIRFilter::filterLoop()
{
    std::cout << "FIRFilter::filterLoop: поток запущен" << std::endl;
    std::cout.flush();

    std::vector<float> window;
    window.reserve(m_windowSize);
    updateCoefficients();

    bool isFirstRun = true;
    float lastValue = 0.0f;
    uint32_t lastTimestamp = 0;
    int processedCount = 0;
    bool hasValidData = false;

    while (!m_stopRequested.load())
    {
        if (!m_inputBuffer)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (m_windowSizeChanged.load())
        {
            m_windowSizeChanged.store(false);
            m_windowSize = m_newWindowSize;
            m_invWindowSize = 1.0f / static_cast<float>(m_windowSize);
            window.clear();
            window.reserve(m_windowSize);

            const DataPoint *last = m_inputBuffer->getLast();
            if (last)
            {
                window.assign(m_windowSize, last->value);
                lastTimestamp = last->timestamp;
                lastValue = last->value;
                hasValidData = true;
            }
            else
            {
                window.assign(m_windowSize, 0.0f);
                lastTimestamp = 0;
                hasValidData = false;
            }

            updateCoefficients();
            std::cout << "FIR: размер окна изменен на " << m_windowSize << std::endl;
            continue;
        }

        const DataPoint *currentPoint = m_inputBuffer->getLast();
        if (!currentPoint)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (currentPoint->timestamp == 0)
        {
            if (!isFirstRun && hasValidData)
            {
                DataPoint filtered;
                filtered.timestamp = lastTimestamp;
                filtered.value = lastValue;
                if (m_outputCallback)
                {
                    m_outputCallback(filtered);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (currentPoint->timestamp != lastTimestamp)
        {
            if (std::isnan(currentPoint->value) || std::isinf(currentPoint->value))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (isFirstRun)
            {
                lastValue = currentPoint->value;
                lastTimestamp = currentPoint->timestamp;
                isFirstRun = false;
                hasValidData = true;
                window.assign(m_windowSize, lastValue);
                std::cout << "FIR: первое значение = " << lastValue << ", timestamp=" << lastTimestamp << std::endl;
                continue;
            }

            lastValue = currentPoint->value;
            lastTimestamp = currentPoint->timestamp;
            hasValidData = true;

            if (window.size() >= m_windowSize)
            {
                std::memmove(window.data(), window.data() + 1, (m_windowSize - 1) * sizeof(float));
                window[m_windowSize - 1] = lastValue;
            }
            else
            {
                window.push_back(lastValue);
            }

            DataPoint filtered;
            filtered.timestamp = currentPoint->timestamp;

            switch (m_algorithm)
            {
            case Algorithm::Boxcar:
                filtered.value = boxcarFilter(window);
                break;
            case Algorithm::Hamming:
                filtered.value = hammingFilter(window);
                break;
            case Algorithm::Blackman:
                filtered.value = blackmanFilter(window);
                break;
            case Algorithm::Median:
                filtered.value = medianFilter(window);
                break;
            case Algorithm::Gaussian:
                filtered.value = gaussianFilter(window);
                break;
            case Algorithm::LowPass:
                filtered.value = lowPassFilter(window);
                break;
            default:
                filtered.value = boxcarFilter(window);
                break;
            }

            if (std::isnan(filtered.value) || std::isinf(filtered.value))
            {
                static int warnCounter = 0;
                if (warnCounter++ % 100 == 0)
                {
                    std::cout << "FIRFilter: ВЫХОДНЫЕ ДАННЫЕ NaN или Inf!" << std::endl;
                    std::cout.flush();
                }
                filtered.value = currentPoint->value;
            }

            if (++processedCount % 10 == 0)
            {
                std::cout << "FIR: timestamp=" << filtered.timestamp
                          << ", value=" << filtered.value
                          << ", windowSize=" << m_windowSize << std::endl;
            }

            if (m_outputCallback)
            {
                m_outputCallback(filtered);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "FIRFilter::filterLoop: поток завершен" << std::endl;
    std::cout.flush();
}

float FIRFilter::boxcarFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    return std::accumulate(window.begin(), window.end(), 0.0f) * m_invWindowSize;
}

float FIRFilter::hammingFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;

    const auto &coeffs = m_cachedCoeffs;
    float sum = std::inner_product(window.begin(), window.end(), coeffs.begin(), 0.0f);
    float weightSum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::blackmanFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;

    const auto &coeffs = m_cachedCoeffs;
    float sum = std::inner_product(window.begin(), window.end(), coeffs.begin(), 0.0f);
    float weightSum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::medianFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;

    std::vector<float> sorted = window;
    size_t n = sorted.size() / 2;
    std::nth_element(sorted.begin(), sorted.begin() + n, sorted.end());
    return sorted[n];
}

float FIRFilter::gaussianFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;

    const auto &coeffs = m_cachedCoeffs;
    float sum = std::inner_product(window.begin(), window.end(), coeffs.begin(), 0.0f);
    float weightSum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::lowPassFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;

    const auto &coeffs = m_cachedCoeffs;
    float sum = std::inner_product(window.begin(), window.end(), coeffs.begin(), 0.0f);
    float weightSum = std::accumulate(coeffs.begin(), coeffs.end(), 0.0f);
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}