#include "DataBuffer.h"

DataBuffer::DataBuffer(size_t capacity)
    : m_capacity(capacity), m_head(0), m_size(0)
{
    std::cout << "DataBuffer: создан с capacity=" << capacity << std::endl;
    m_data.resize(capacity);
}

void DataBuffer::push(const DataPoint &point)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data[m_head] = point;
    m_head = (m_head + 1) % m_capacity;
    if (m_size < m_capacity)
        m_size++;
}

size_t DataBuffer::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_size;
}

std::vector<DataPoint> DataBuffer::getAll() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DataPoint> result;
    result.reserve(m_size);
    size_t start = (m_head - m_size + m_capacity) % m_capacity;
    for (size_t i = 0; i < m_size; ++i)
    {
        result.push_back(m_data[(start + i) % m_capacity]);
    }
    return result;
}

void DataBuffer::setCapacity(size_t capacity)
{
    std::cout << "DataBuffer::setCapacity: начало, capacity=" << capacity << std::endl;
    std::cout.flush();

    std::lock_guard<std::mutex> lock(m_mutex);

    std::cout << "DataBuffer::setCapacity: мьютекс захвачен" << std::endl;
    std::cout.flush();

    std::vector<DataPoint> oldData;
    oldData.reserve(m_size);
    for (size_t i = 0; i < m_size; ++i)
    {
        size_t idx = (m_head - m_size + i + m_capacity) % m_capacity;
        oldData.push_back(m_data[idx]);
    }

    std::cout << "DataBuffer::setCapacity: старых данных=" << oldData.size() << std::endl;
    std::cout.flush();

    m_capacity = capacity;
    m_data.clear();
    m_data.resize(capacity);
    m_head = 0;
    m_size = std::min(oldData.size(), capacity);

    for (size_t i = 0; i < m_size; ++i)
    {
        m_data[i] = oldData[oldData.size() - m_size + i];
    }

    std::cout << "DataBuffer::setCapacity: завершено" << std::endl;
    std::cout.flush();
}