#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <vector>
#include <mutex>
#include <cstdint>
#include <QMetaType>

struct DataPoint
{
    uint32_t timestamp;
    float value;
};

Q_DECLARE_METATYPE(DataPoint)

class DataBuffer
{
public:
    explicit DataBuffer(size_t capacity = 1000);

    void push(const DataPoint &point);
    size_t size() const;
    size_t capacity() const { return m_capacity; }

    std::vector<DataPoint> getData() const;
    std::pair<const DataPoint *, size_t> getRange() const;
    const DataPoint *getLast() const;

    void setCapacity(size_t capacity);

private:
    mutable std::mutex m_mutex;
    std::vector<DataPoint> m_data;
    size_t m_capacity;
    size_t m_head;
    size_t m_size;
};

#endif