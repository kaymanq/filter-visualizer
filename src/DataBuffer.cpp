#include "DataBuffer.h"
#include <iostream>

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

std::vector<DataPoint> DataBuffer::getData() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DataPoint> result;
    result.reserve(m_size);

    if (m_size == 0)
    {
        return result;
    }

    size_t start = (m_head + m_capacity - m_size) % m_capacity;
    for (size_t i = 0; i < m_size; ++i)
    {
        result.push_back(m_data[(start + i) % m_capacity]);
    }

    return result;
}

std::pair<const DataPoint *, size_t> DataBuffer::getRange() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_size == 0)
    {
        return {nullptr, 0};
    }

    size_t start = (m_head + m_capacity - m_size) % m_capacity;
    return {&m_data[start], m_size};
}

const DataPoint *DataBuffer::getLast() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_size == 0)
    {
        return nullptr;
    }
    size_t lastIndex = (m_head == 0) ? (m_capacity - 1) : (m_head - 1);
    return &m_data[lastIndex];
}

void DataBuffer::setCapacity(size_t capacity)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (capacity == m_capacity)
        return;

    std::vector<DataPoint> oldData;
    oldData.reserve(m_size);
    size_t start = (m_head + m_capacity - m_size) % m_capacity;
    for (size_t i = 0; i < m_size; ++i)
    {
        oldData.emplace_back(m_data[(start + i) % m_capacity]);
    }

    m_capacity = capacity;
    m_data.clear();
    m_data.resize(capacity);
    m_head = 0;
    m_size = std::min(oldData.size(), capacity);

    for (size_t i = 0; i < m_size; ++i)
    {
        m_data[i] = oldData[oldData.size() - m_size + i];
    }
}