#include "IIRFilter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

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

IIRFilter::IIRFilter()
{
    std::cout << "IIRFilter: создан" << std::endl;
}

IIRFilter::~IIRFilter()
{
    std::cout << "IIRFilter: деструктор" << std::endl;
    stop();
}

void IIRFilter::resetState()
{
    std::cout << "IIRFilter: СБРОС СОСТОЯНИЯ" << std::endl;
    m_prevOutput = 0.0f;
    m_butterworthState = ButterworthState{};

    if (m_inputBuffer)
    {
        auto [data, dataSize] = m_inputBuffer->getRange();
        if (dataSize > 0)
        {
            float currentValue = data[dataSize - 1].value;
            m_prevOutput = currentValue;
            m_butterworthState.x1 = currentValue;
            m_butterworthState.y1 = currentValue;
            std::cout << "IIRFilter: состояние инициализировано значением " << currentValue << std::endl;
        }
    }
    std::cout.flush();
}

void IIRFilter::updateButterworthCoeffs()
{
    double fc = m_cutoffFreq;
    if (fc < 0.01)
        fc = 0.01;
    if (fc > 0.49)
        fc = 0.49;

    double omega = std::tan(M_PI * fc);
    double omega2 = omega * omega;
    double sqrt2 = 1.4142135623730951;

    double norm = 1.0 / (1.0 + sqrt2 * omega + omega2);
    m_b0 = norm;
    m_b1 = 2.0 * norm;
    m_b2 = norm;
    m_a1 = 2.0 * (omega2 - 1.0) * norm;
    m_a2 = (1.0 - sqrt2 * omega + omega2) * norm;
}

void IIRFilter::start(DataBuffer *inputBuffer)
{
    std::cout << "IIRFilter::start: начало" << std::endl;

    if (m_running.load())
    {
        std::cout << "IIRFilter::start: уже запущен" << std::endl;
        return;
    }

    m_inputBuffer = inputBuffer;
    resetState();
    updateButterworthCoeffs();
    m_running.store(true);
    m_stopRequested.store(false);

    std::cout << "IIRFilter::start: создаем поток" << std::endl;
    m_thread = std::thread(&IIRFilter::filterLoop, this);

    std::cout << "IIRFilter::start: поток создан, выходим" << std::endl;
}

void IIRFilter::stop()
{
    std::cout << "IIRFilter::stop: начало" << std::endl;

    if (m_running.load())
    {
        m_stopRequested.store(true);
        m_running.store(false);
        if (m_thread.joinable())
        {
            std::cout << "IIRFilter::stop: ждем завершения потока..." << std::endl;
            m_thread.join();
            std::cout << "IIRFilter::stop: поток завершен" << std::endl;
        }
    }

    std::cout << "IIRFilter::stop: завершено" << std::endl;
}

void IIRFilter::setAlpha(float alpha)
{
    m_alpha = clamp(alpha, 0.01f, 1.0f);
    std::cout << "IIRFilter: alpha = " << m_alpha << std::endl;
}

void IIRFilter::setCutoffFrequency(float freq)
{
    m_cutoffFreq = clamp(freq, 0.01f, 0.99f);
    updateButterworthCoeffs();
    std::cout << "IIRFilter: cutoff frequency = " << m_cutoffFreq << std::endl;
}

void IIRFilter::setAlgorithm(Algorithm algo)
{
    m_algorithm = algo;
    resetState();
    updateButterworthCoeffs();

    std::cout << "IIRFilter: алгоритм изменен на ";
    switch (algo)
    {
    case Algorithm::Exponential:
        std::cout << "Exponential";
        break;
    case Algorithm::Butterworth:
        std::cout << "Butterworth";
        break;
    default:
        std::cout << "Unknown";
        break;
    }
    std::cout << " (состояние сброшено)" << std::endl;
    std::cout.flush();
}

void IIRFilter::setOutputCallback(OutputCallback callback)
{
    m_outputCallback = callback;
}

void IIRFilter::filterLoop()
{
    std::cout << "IIRFilter::filterLoop: поток запущен" << std::endl;
    std::cout.flush();

    bool firstRun = true;
    uint32_t lastTimestamp = 0;
    int processedCount = 0;

    while (!m_stopRequested.load())
    {
        if (!m_inputBuffer)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        const auto &data = m_inputBuffer->getData();
        if (data.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const DataPoint &point = data.back();

        if (point.timestamp != lastTimestamp)
        {
            if (firstRun)
            {
                lastTimestamp = point.timestamp;
                firstRun = false;
                m_prevOutput = point.value;
                std::cout << "IIR: первое значение = " << point.value << ", timestamp=" << lastTimestamp << std::endl;
                continue;
            }

            lastTimestamp = point.timestamp;
            float output;

            if (m_algorithm == Algorithm::Exponential)
            {
                output = m_alpha * point.value + (1.0f - m_alpha) * m_prevOutput;
                m_prevOutput = output;
            }
            else
            {
                output = butterworthFilter(point.value);
            }

            if (std::isnan(output) || std::isinf(output))
            {
                static int warnCounter = 0;
                if (warnCounter++ % 100 == 0)
                {
                    std::cout << "IIRFilter: ВЫХОДНЫЕ ДАННЫЕ NaN или Inf! Возвращаем входные данные" << std::endl;
                    std::cout.flush();
                }
                output = point.value;
            }

            if (++processedCount % 10 == 0)
            {
                std::cout << "IIR: timestamp=" << point.timestamp
                          << ", value=" << output << std::endl;
            }

            DataPoint filtered;
            filtered.timestamp = point.timestamp;
            filtered.value = output;

            if (m_outputCallback)
            {
                m_outputCallback(filtered);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "IIRFilter::filterLoop: поток завершен" << std::endl;
    std::cout.flush();
}

float IIRFilter::exponentialFilter(float input)
{
    float output = m_alpha * input + (1.0f - m_alpha) * m_prevOutput;
    m_prevOutput = output;
    return output;
}

float IIRFilter::butterworthFilter(float input)
{
    if (std::isnan(input) || std::isinf(input))
    {
        return 0.0f;
    }

    float output = static_cast<float>(
        m_b0 * input + m_b1 * m_butterworthState.x1 + m_b2 * m_butterworthState.x2 - m_a1 * m_butterworthState.y1 - m_a2 * m_butterworthState.y2);

    m_butterworthState.x2 = m_butterworthState.x1;
    m_butterworthState.x1 = input;
    m_butterworthState.y2 = m_butterworthState.y1;
    m_butterworthState.y1 = output;

    return output;
}