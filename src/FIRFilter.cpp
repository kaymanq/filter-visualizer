#include "FIRFilter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

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
    m_windowSize = (size % 2 == 0) ? size + 1 : size;
    std::cout << "FIRFilter: window size = " << m_windowSize << std::endl;
}

void FIRFilter::setAlgorithm(Algorithm algo)
{
    m_algorithm = algo;
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
    std::cout << "FIRFilter: cutoff frequency = " << m_cutoffFreq << std::endl;
}

void FIRFilter::setOutputCallback(OutputCallback callback)
{
    m_outputCallback = callback;
}

void FIRFilter::filterLoop()
{
    std::cout << "FIRFilter::filterLoop: поток запущен" << std::endl;
    std::cout.flush();

    std::vector<float> window;
    window.reserve(m_windowSize);

    // ==========================================
    // СЧЕТЧИК ДЛЯ СИНХРОНИЗАЦИИ
    // ==========================================
    int processCounter = 0;
    const int PROCESS_EVERY_N = 2; // Обрабатываем каждые 2 вызова

    while (!m_stopRequested.load())
    {
        auto data = m_inputBuffer->getAll();

        if (data.size() < m_windowSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // ==========================================
        // ПРОПУСКАЕМ КАЖДЫЙ ВТОРОЙ ВЫЗОВ
        // ==========================================
        processCounter++;
        if (processCounter % PROCESS_EVERY_N != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        size_t start = data.size() - m_windowSize;
        window.clear();

        for (size_t i = start; i < data.size(); ++i)
        {
            window.push_back(data[i].value);
        }

        DataPoint filtered;

        // ==========================================
        // ЦЕНТР ОКНА
        // ==========================================
        size_t centerIndex = start + m_windowSize / 2;
        if (centerIndex < data.size())
        {
            filtered.timestamp = data[centerIndex].timestamp;
        }
        else
        {
            filtered.timestamp = data.back().timestamp;
        }

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
            filtered.value = data.back().value;
        }

        if (m_outputCallback)
        {
            m_outputCallback(filtered);
        }

        // ==========================================
        // ЗАДЕРЖКА ДЛЯ СИНХРОНИЗАЦИИ
        // ==========================================
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "FIRFilter::filterLoop: поток завершен" << std::endl;
    std::cout.flush();
}

// ==========================================
// РЕАЛИЗАЦИИ АЛГОРИТМОВ
// ==========================================

float FIRFilter::boxcarFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    float sum = 0.0f;
    for (float v : window)
    {
        sum += v;
    }
    return sum / static_cast<float>(window.size());
}

float FIRFilter::hammingFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    auto coeffs = getWindowCoefficients();
    float sum = 0.0f;
    float weightSum = 0.0f;
    for (size_t i = 0; i < window.size(); ++i)
    {
        sum += window[i] * coeffs[i];
        weightSum += coeffs[i];
    }
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::blackmanFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    const float a0 = 0.42f;
    const float a1 = 0.5f;
    const float a2 = 0.08f;
    float sum = 0.0f;
    float weightSum = 0.0f;
    int N = static_cast<int>(window.size());
    for (int i = 0; i < N; ++i)
    {
        double angle1 = 2.0 * M_PI * i / (N - 1);
        double angle2 = 4.0 * M_PI * i / (N - 1);
        float w = a0 - a1 * static_cast<float>(std::cos(angle1)) + a2 * static_cast<float>(std::cos(angle2));
        sum += window[i] * w;
        weightSum += w;
    }
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::medianFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    std::vector<float> sorted = window;
    std::sort(sorted.begin(), sorted.end());
    return sorted[sorted.size() / 2];
}

float FIRFilter::gaussianFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    float sigma = static_cast<float>(window.size()) / 6.0f;
    float sum = 0.0f;
    float weightSum = 0.0f;
    int N = static_cast<int>(window.size());
    int center = N / 2;
    for (int i = 0; i < N; ++i)
    {
        float x = static_cast<float>(i - center);
        float w = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += window[i] * w;
        weightSum += w;
    }
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

float FIRFilter::lowPassFilter(const std::vector<float> &window) const
{
    if (window.empty())
        return 0.0f;
    int N = static_cast<int>(window.size());
    float sum = 0.0f;
    float weightSum = 0.0f;
    float fc = static_cast<float>(m_cutoffFreq);
    int center = N / 2;
    float cutoff = fc * 2.0f;
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
        float w = sinc * hamming;
        sum += window[i] * w;
        weightSum += w;
    }
    return (weightSum > 0.0f) ? sum / weightSum : 0.0f;
}

std::vector<float> FIRFilter::getWindowCoefficients() const
{
    std::vector<float> coeffs(m_windowSize);
    int N = static_cast<int>(m_windowSize);
    for (int i = 0; i < N; ++i)
    {
        double angle = 2.0 * M_PI * i / (N - 1);
        switch (m_algorithm)
        {
        case Algorithm::Hamming:
            coeffs[i] = 0.54f - 0.46f * static_cast<float>(std::cos(angle));
            break;
        case Algorithm::Blackman:
            coeffs[i] = 0.42f - 0.5f * static_cast<float>(std::cos(angle)) + 0.08f * static_cast<float>(std::cos(2.0 * angle));
            break;
        default:
            coeffs[i] = 1.0f;
            break;
        }
    }
    return coeffs;
}