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

void IIRFilter::start(DataBuffer *inputBuffer)
{
    std::cout << "IIRFilter::start: начало" << std::endl;

    if (m_running.load())
    {
        std::cout << "IIRFilter::start: уже запущен" << std::endl;
        return;
    }

    m_inputBuffer = inputBuffer;
    m_prevOutput = 0.0f;
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

void IIRFilter::setOrder(int order)
{
    m_order = clamp(order, 1, 4);
    std::cout << "IIRFilter: order = " << m_order << std::endl;
}

void IIRFilter::setCutoffFrequency(float freq)
{
    m_cutoffFreq = clamp(freq, 0.01f, 0.99f);
    std::cout << "IIRFilter: cutoff frequency = " << m_cutoffFreq << std::endl;
}

void IIRFilter::setAlgorithm(Algorithm algo)
{
    m_algorithm = algo;
    std::cout << "IIRFilter: алгоритм изменен на ";
    switch (algo)
    {
    case Algorithm::Exponential:
        std::cout << "Exponential";
        break;
    case Algorithm::Butterworth:
        std::cout << "Butterworth";
        break;
    case Algorithm::Chebyshev:
        std::cout << "Chebyshev";
        break;
    case Algorithm::Bessel:
        std::cout << "Bessel";
        break;
    default:
        std::cout << "Unknown";
        break;
    }
    std::cout << std::endl;
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
            float output = 0.0f;

            switch (m_algorithm)
            {
            case Algorithm::Exponential:
                output = exponentialFilter(point.value);
                break;
            case Algorithm::Butterworth:
                output = butterworthFilter(point.value);
                break;
            case Algorithm::Chebyshev:
                output = chebyshevFilter(point.value);
                break;
            case Algorithm::Bessel:
                output = besselFilter(point.value);
                break;
            default:
                output = exponentialFilter(point.value);
                break;
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

float IIRFilter::butterworthFilter(float input)
{
    double fc = static_cast<double>(m_cutoffFreq);

    if (fc < 0.01)
        fc = 0.01;
    if (fc > 0.49)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Butterworth: частота среза " << fc << " > 0.49, ограничиваем до 0.49" << std::endl;
            std::cout.flush();
        }
        fc = 0.49;
    }

    double omega = std::tan(M_PI * fc);
    double omega2 = omega * omega;

    if (std::isnan(omega) || std::isinf(omega) || omega2 > 1e10)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Butterworth: omega2 = " << omega2 << " - переполнение, пропускаем фильтр" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    double sqrt2 = std::sqrt(2.0);

    double a0 = omega2;
    double a1 = sqrt2 * omega;
    double a2 = 1.0;

    if (std::isnan(a0) || std::isinf(a0) || a0 == 0.0)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Butterworth: a0 = " << a0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    double norm = 1.0 / a0;
    double b0 = norm;
    double b1 = 2.0 * norm;
    double b2 = norm;

    if (std::isnan(b0) || std::isinf(b0))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Butterworth: b0 = " << b0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    float output = static_cast<float>(
        b0 * input + b1 * m_butterworthState.x1 + b2 * m_butterworthState.x2 - a1 * m_butterworthState.y1 - a2 * m_butterworthState.y2);

    if (std::isnan(output) || std::isinf(output))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Butterworth: ВЫХОДНЫЕ ДАННЫЕ NaN или Inf!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    // Обновляем состояние
    m_butterworthState.x2 = m_butterworthState.x1;
    m_butterworthState.x1 = input;
    m_butterworthState.y2 = m_butterworthState.y1;
    m_butterworthState.y1 = output;

    return output;
}

float IIRFilter::chebyshevFilter(float input)
{
    double fc = static_cast<double>(m_cutoffFreq);

    if (fc < 0.01)
        fc = 0.01;
    if (fc > 0.45)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Chebyshev: частота среза " << fc << " > 0.45, ограничиваем до 0.45" << std::endl;
            std::cout.flush();
        }
        fc = 0.45;
    }

    double epsilon = 0.3493; // 0.5 dB ripple
    double omega = std::tan(M_PI * fc);
    double omega2 = omega * omega;

    // Проверка на переполнение
    if (std::isnan(omega) || std::isinf(omega) || omega2 > 1e10)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Chebyshev: omega2 = " << omega2 << " - переполнение, пропускаем фильтр" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    double ep2 = epsilon * epsilon;
    double sqrt2 = std::sqrt(2.0);

    double a0 = omega2;
    double a1 = sqrt2 * omega / std::sqrt(1 + ep2);
    double a2 = 1 + ep2;

    if (std::isnan(a0) || std::isinf(a0) || a0 == 0.0)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Chebyshev: a0 = " << a0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    double norm = 1.0 / a0;
    double b0 = norm;
    double b1 = 2.0 * norm;
    double b2 = norm;

    if (std::isnan(b0) || std::isinf(b0))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Chebyshev: b0 = " << b0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    float output = static_cast<float>(
        b0 * input + b1 * m_chebyshevState.x1 + b2 * m_chebyshevState.x2 - a1 * m_chebyshevState.y1 - a2 * m_chebyshevState.y2);

    if (std::isnan(output) || std::isinf(output))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Chebyshev: ВЫХОДНЫЕ ДАННЫЕ NaN или Inf!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    m_chebyshevState.x2 = m_chebyshevState.x1;
    m_chebyshevState.x1 = input;
    m_chebyshevState.y2 = m_chebyshevState.y1;
    m_chebyshevState.y1 = output;

    return output;
}

float IIRFilter::besselFilter(float input)
{
    // Проверка входных данных
    if (std::isnan(input) || std::isinf(input))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: ВХОДНЫЕ ДАННЫЕ NaN или Inf!" << std::endl;
            std::cout.flush();
        }
        return 0.0f;
    }

    double fc = static_cast<double>(m_cutoffFreq);

    if (fc > 0.4)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: частота среза " << fc << " > 0.4, ограничиваем до 0.4" << std::endl;
            std::cout.flush();
        }
        fc = 0.4;
    }
    if (fc < 0.01)
        fc = 0.01;

    double omega = std::tan(M_PI * fc);
    double omega2 = omega * omega;

    // Проверка на слишком малые значения
    if (omega2 < 1e-10)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: omega2 слишком мало = " << omega2 << ", пропускаем фильтр" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    // Проверка на переполнение
    if (std::isnan(omega) || std::isinf(omega) || omega2 > 1e10)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: omega2 = " << omega2 << " - переполнение, пропускаем фильтр" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    // Коэффициенты фильтра Бесселя
    double a0 = 3.0 * omega2;
    double a1 = 3.0 * omega;
    double a2 = 1.0;

    if (std::isnan(a0) || std::isinf(a0) || a0 == 0.0)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: a0 = " << a0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    double norm = 1.0 / a0;
    double b0 = norm;
    double b1 = 2.0 * norm;
    double b2 = norm;

    if (std::isnan(b0) || std::isinf(b0))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: b0 = " << b0 << " - невалидный коэффициент!" << std::endl;
            std::cout.flush();
        }
        return input;
    }

    float output = static_cast<float>(
        b0 * input + b1 * m_besselState.x1 + b2 * m_besselState.x2 - a1 * m_besselState.y1 - a2 * m_besselState.y2);

    if (std::isnan(output) || std::isinf(output))
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "Bessel: ВЫХОДНЫЕ ДАННЫЕ NaN или Inf! a0=" << a0 << ", b0=" << b0 << ", omega=" << omega << std::endl;
            std::cout.flush();
        }
        return input;
    }

    static int counter = 0;
    if (++counter % 500 == 0)
    {
        std::cout << "Bessel: fc=" << fc
                  << ", input=" << input
                  << ", output=" << output
                  << ", omega=" << omega
                  << ", a0=" << a0
                  << ", b0=" << b0 << std::endl;
        std::cout.flush();
    }

    m_besselState.x2 = m_besselState.x1;
    m_besselState.x1 = input;
    m_besselState.y2 = m_besselState.y1;
    m_besselState.y1 = output;

    return output;
}