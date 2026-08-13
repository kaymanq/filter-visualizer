#include "IIRFilter.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>

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

    if (m_inputBuffer)
    {
        auto data = m_inputBuffer->getAll();
        if (!data.empty())
        {
            float currentValue = data.back().value;
            m_prevOutput = currentValue;
            std::cout << "IIRFilter: состояние инициализировано значением " << currentValue << std::endl;
        }
    }
    std::cout.flush();
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

void IIRFilter::setAlgorithm(Algorithm algo)
{
    m_algorithm = algo;
    resetState();

    std::cout << "IIRFilter: алгоритм изменен на Exponential (состояние сброшено)" << std::endl;
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

    while (!m_stopRequested.load())
    {
        auto data = m_inputBuffer->getAll();

        if (!data.empty())
        {
            const auto &point = data.back();
            float output = exponentialFilter(point.value);

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

            DataPoint filtered;
            filtered.timestamp = point.timestamp;
            filtered.value = output;

            if (m_outputCallback)
            {
                m_outputCallback(filtered);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
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